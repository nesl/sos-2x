/**
 * \file minielflib.c
 * \brief Library functions to write a Mini-Elf file
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

/**
 * \todo
 * -# Handle Endian-ness during read and write
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <libelf.h>
#include <string.h>
#include <sock_utils.h>
#include <minielflib.h>
#include <minielf.h>
#include <minielfendian.h>
#include <stringconst.h>

#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif//DBGMODE


static int insertScn(Melf_Scn* scn, Melf* pmelf);
static inline int scnOrder(Melf_Scn* scn);
static int _readMelf(Melf* MelfDesc);
static void cleanupMelf(Melf* MelfDesc);

// ---- ENTOH ----
static void entoh_symboltable(Melf_Scn* symscn);
static void entoh_relatable(Melf_Scn* relascn);
//static void entoh_progbits(Melf_Scn* progscn);
// ---- EHTON ----
static Melf_Sym* ehton_symboltable(Melf_Scn* symscn);
static Melf_Rela* ehton_relatable(Melf_Scn* relascn);
//static void* ehton_progbits(Melf_Scn* progscn);

#ifdef DBGMODE
static int printSecOrder(Melf* MelfDesc);
Melf* dbgmelf;
#endif//DBGMODE

int ltcmpscn(list_t* lhop, list_t* rhop);
void swapscn(list_t* x, list_t* y);


//-----------------------------------------------------------
// API IMPLEMENTATION
//-----------------------------------------------------------
Melf* melf_new(int fd)
{
  Melf* pMelf;

  pMelf = malloc(sizeof(Melf));
  if (NULL == pMelf) return NULL;
  pMelf->m_fd = fd;
  pMelf->m_mhdr.m_shnum = 0;
  pMelf->m_scn_1 = NULL;
  pMelf->m_scn_n = NULL;

#ifdef DBGMODE
  dbgmelf = pMelf;
#endif//DBGMODE

  return pMelf;
}
//-----------------------------------------------------------
Melf_Scn* melf_new_scn(Melf* MelfDesc)
{
  Melf_Scn* pmscn;
  if (MelfDesc == NULL) return NULL; // There is no header
  pmscn = malloc(sizeof(Melf_Scn));
  if (NULL == pmscn) return NULL;
  pmscn->m_data = NULL;
  if (insertScn(pmscn, MelfDesc) != 0 ){
    free(pmscn);
    return NULL;
  }
  pmscn->m_shdr.sh_id = MelfDesc->m_mhdr.m_shnum;
  MelfDesc->m_mhdr.m_shnum++;
  return pmscn;
}
//-----------------------------------------------------------
void melf_setModHdrSymNdx(Melf* MelfDesc, Melf_Word modhdrndx)
{
  MelfDesc->m_mhdr.m_modhdrndx = modhdrndx;
  return;
}
//-----------------------------------------------------------
void melf_sortScn(Melf* MelfDesc)

{  
  DEBUG("===== Sorting MELF File Write === \n");
#ifdef DBGMODE
  printf("Unsorted: ");
  printSecOrder(dbgmelf);
#endif
  qsortlist((list_t*)MelfDesc->m_scn_1, (list_t*)MelfDesc->m_scn_n,
	    ltcmpscn, swapscn);
#ifdef DBGMODE
  printf("Sorted: ");
  printSecOrder(dbgmelf);
#endif
  return;
}
//-----------------------------------------------------------
void melf_setScnOffsets(Melf* MelfDesc)
{
  Melf_Off curroffset;
  Melf_Scn* pCurrScn;
  curroffset = 0;
  DEBUG("===== Assigning Section Offsets  === \n");
  // Initial Offset
  curroffset += sizeof(Melf_Mhdr) + (sizeof(Melf_Shdr) 
				     * (MelfDesc->m_mhdr.m_shnum));
  for (pCurrScn = MelfDesc->m_scn_1; pCurrScn != NULL; 
       pCurrScn = (Melf_Scn*)pCurrScn->list.next){
    pCurrScn->m_shdr.sh_offset = curroffset;
    curroffset += pCurrScn->m_shdr.sh_size;    
    DEBUG("Section %2d @ Offset %d\n", pCurrScn->m_shdr.sh_id, pCurrScn->m_shdr.sh_offset);
  }
  return;
}
//-----------------------------------------------------------
int melf_write(Melf* MelfDesc)
{
  /* TODO - Fix the endian-ness before writing */
  Melf_Mhdr mhdr;
  Melf_Shdr shdr;
  Melf_Scn* pCurrScn;

  DEBUG("===== Starting MELF File Write === \n");
  // Fix the endian-ness of the MELF Header
  memcpy(&mhdr, &(MelfDesc->m_mhdr), sizeof(Melf_Mhdr));
  ehton_Mhdr(&mhdr);
  // First write the MELF Header
  if (writen(MelfDesc->m_fd, (void*)(&mhdr), 
	     sizeof(Melf_Mhdr)) < 0){ 
    fprintf(stderr, "melf_write (Header) -> writen: Error writing file\n");
    return -1;
  };
  DEBUG("MELF Header written: %d bytes.\n", (int)sizeof(Melf_Mhdr));

  // Next write all the MELF Section Headers
  for (pCurrScn = MelfDesc->m_scn_1; pCurrScn != NULL; 
       pCurrScn = (Melf_Scn*)pCurrScn->list.next){
    memcpy(&shdr, &(pCurrScn->m_shdr), sizeof(Melf_Shdr));
    ehton_Shdr(&shdr);
    if (writen(MelfDesc->m_fd, (void*)&shdr,
	       sizeof(Melf_Shdr)) < 0){
      fprintf(stderr, "melf_write (Sections) -> writen: Error writing file\n");
      return -1;
    }
    DEBUG("Section Header %2d written: %d bytes\n", pCurrScn->m_shdr.sh_id, (int)sizeof(Melf_Shdr));
  }
  // Write the data portions of each section
  for (pCurrScn = MelfDesc->m_scn_1; pCurrScn != NULL; 
       pCurrScn = (Melf_Scn*)pCurrScn->list.next){
    void* mdata;
    DEBUG("Section Data %02d Written: %5d bytes @ offset %5d\n", pCurrScn->m_shdr.sh_id, pCurrScn->m_shdr.sh_size, pCurrScn->m_shdr.sh_offset);
    if (0 == pCurrScn->m_shdr.sh_size) continue;

    // Fix the endian-ness of symbol table and relocation section
    switch (pCurrScn->m_shdr.sh_type){
    case SHT_SYMTAB: mdata = (void*)ehton_symboltable(pCurrScn); break;
    case SHT_RELA:   mdata = (void*)ehton_relatable(pCurrScn); break;
      //    case SHT_PROGBITS: mdata = (void*)ehton_progbits(pCurrScn); break;
    default: mdata = (void*)pCurrScn->m_data; break;
    }

    if (writen(MelfDesc->m_fd, mdata, pCurrScn->m_shdr.sh_size) < 0){
      fprintf(stderr, "melf_write (Data) -> writen: Error writing file\n");
      return -1;
    }

    if (mdata != pCurrScn->m_data) 
      free(mdata);

  }
  // Done writing MELF File
  return 0;
}
//-----------------------------------------------------------
Melf* melf_begin(int fd)
{
  Melf* MelfDesc;
  if ((MelfDesc = (Melf*)malloc(sizeof(Melf))) == NULL){
    return NULL;
  }
  MelfDesc->m_fd = fd;
  if (_readMelf(MelfDesc) < 0){
    free(MelfDesc);
    return NULL;
  }
  return MelfDesc;
}
//-----------------------------------------------------------
Melf_Mhdr* melf_getmhdr(Melf* MelfDesc)
{
  return (Melf_Mhdr*)&(MelfDesc->m_mhdr);
}
//-----------------------------------------------------------
Melf_Scn* melf_getscn(Melf* MelfDesc, int sh_id)
{
  Melf_Scn* pCurrScn;
  for (pCurrScn = MelfDesc->m_scn_1; pCurrScn != NULL; pCurrScn = (Melf_Scn*)pCurrScn->list.next){
    if (sh_id == pCurrScn->m_shdr.sh_id){
      return pCurrScn;
    }
  }
  return NULL;
}
//-----------------------------------------------------------
Melf_Scn* melf_nextscn(Melf* MelfDesc, Melf_Scn* scn)
{
  if (NULL == scn)
    return MelfDesc->m_scn_1;
  return (Melf_Scn*)(scn->list.next);
} 
//-----------------------------------------------------------
Melf_Shdr* melf_getshdr(Melf_Scn* scn)
{
  if (NULL == scn)
    return NULL;
  return &(scn->m_shdr);
}
//-----------------------------------------------------------
#define ASSGNDTYPE(stype, dtype, dsize) case stype:   \
  {						      \
    mdata->d_type = dtype;			      \
    mdata->d_numData = mdata->d_size/dsize;	      \
    break;					      \
  }						      \
  

Melf_Data* melf_getdata(Melf_Scn* scn)
{
  Melf_Data* mdata;
  if (NULL == scn){
    fprintf(stderr, "melf_getdata: Input section is empty.\n");
    return NULL;
  }
  if ((mdata = malloc(sizeof(Melf_Data))) == NULL){
    fprintf(stderr, "melf_getdata: Error allocating memory.\n");
    return NULL;
  }

  mdata->d_buf = scn->m_data;
  mdata->d_size = scn->m_shdr.sh_size;

  switch (scn->m_shdr.sh_type){
    ASSGNDTYPE( SHT_NOBITS, ELF_T_BYTE, sizeof(char));
    ASSGNDTYPE( SHT_PROGBITS, ELF_T_BYTE, sizeof(char));
    ASSGNDTYPE( SHT_RELA, ELF_T_RELA, sizeof(Melf_Rela));
    ASSGNDTYPE( SHT_STRTAB, ELF_T_BYTE, sizeof(Melf_Rela));
    ASSGNDTYPE( SHT_SYMTAB, ELF_T_SYM, sizeof(Melf_Sym));
  default:
    {
      mdata->d_type = ELF_T_BYTE;
      mdata->d_numData = (mdata->d_size/sizeof(char));
      break;
    }
  }


  return mdata;
}
//-----------------------------------------------------------
int melf_idscn(Melf_Scn* scn)
{
  if (NULL == scn)
    return -1;
  return (scn->m_shdr.sh_id);
}
//-----------------------------------------------------------
int melf_add_data_to_scn(Melf_Scn* scn, Melf_Data* mdata)
{
  if (NULL == scn){
    fprintf(stderr, "Section pointer is NULL.\n");
    return -1;
  }

  if (NULL == mdata){
    fprintf(stderr, "Data pointer to add to section is NULL.\n");
    return -1;
  }

  if (scn->m_data != NULL){
    fprintf(stderr, "MELF section already contains data.\n");
    return -1;
  }

  scn->m_shdr.sh_size = mdata->d_size;
  scn->m_data = mdata->d_buf;
  return 0;
}


//-----------------------------------------------------------
// STATIC FUNCTIONS
//-----------------------------------------------------------
static int _readMelf(Melf* MelfDesc)
{
  int fd;
  int i;
  Melf_Scn *currScn;
  long fdoffset;
  int  datasize;

  DEBUG("===== Starting MELF File Read === \n");
  // Get the file descriptor
  fd = MelfDesc->m_fd;

  // Get the file pointer to the start of file
  if (lseek(fd, 0L, SEEK_SET) < 0){
    perror("_readMelf -> lseek:");
    return -1;
  }  

  // Read the Melf Header
  if (readn(fd, (void*)(&MelfDesc->m_mhdr), sizeof(Melf_Mhdr)) 
      != sizeof(Melf_Mhdr)){ 
    fprintf(stderr, "_readMelf (Header) -> readn: Could not read all the bytes\n");
    return -1;
  }
  entoh_Mhdr(&MelfDesc->m_mhdr);
  DEBUG("Read MELF Header: %d bytes\n", (int)sizeof(Melf_Mhdr)); 



  // Initialize link list
  MelfDesc->m_scn_1 = NULL;
  MelfDesc->m_scn_n = NULL;

  // Read all the section headers
  for (i = 0; i < MelfDesc->m_mhdr.m_shnum; i++){

    // Allocate memory for section header
    if ((currScn = (Melf_Scn*)malloc(sizeof(Melf_Scn))) == NULL){
      cleanupMelf(MelfDesc);
      fprintf(stderr, "_readMelf: Out of memory while allocating a section header\n");
      return -1;
    }

    // Read the section header from MELF file
    if (readn(fd, (void*)&(currScn->m_shdr), sizeof(Melf_Shdr))
	!= sizeof(Melf_Shdr)){ 
      cleanupMelf(MelfDesc);
      fprintf(stderr, "_readMelf (Section Header) -> readn: Could not read all the bytes\n");
      return -1;
    }
    entoh_Shdr(&(currScn->m_shdr));
    DEBUG("Section Header %2d read: %d bytes\n", currScn->m_shdr.sh_id, (int)sizeof(Melf_Shdr));
    // Set the data pointer to NULL
    currScn->m_data = NULL;

    // Insert Section
    insertScn(currScn, MelfDesc);
  }

  
  // Read all the data
  for (currScn = MelfDesc->m_scn_1; currScn != NULL; currScn = (Melf_Scn*)currScn->list.next){

    // Get the size and offset of the data
    fdoffset = (long)currScn->m_shdr.sh_offset;
    datasize = (int)currScn->m_shdr.sh_size;

    DEBUG("Section Data %02d Read: %5d bytes from offset %5d\n", currScn->m_shdr.sh_id, datasize, (int)fdoffset);

    if (0 == datasize){
      continue;
    }

    // Allocate memory for data
    if ((currScn->m_data = malloc(datasize)) == NULL){
      cleanupMelf(MelfDesc);
      fprintf(stderr, "_readMelf: Out of memory while allocating data portion\n");
      return -1;
    }

    // Get the file pointer to the start of data
    if (lseek(fd, fdoffset, SEEK_SET) < 0){
      cleanupMelf(MelfDesc);
      perror("_readMelf (reading Data) -> lseek:");
      return -1;
    }

    // Read the data into buffer
    if (readn(fd, currScn->m_data, datasize) != datasize){
      cleanupMelf(MelfDesc);
      fprintf(stderr, "_readMelf (reading Data) -> readn: Could not read all the bytes\n");
      return -1;
    }

    // Fix endian-ness
    switch (currScn->m_shdr.sh_type){
    case SHT_SYMTAB: entoh_symboltable(currScn); break;
    case SHT_RELA: entoh_relatable(currScn); break;
      //    case SHT_PROGBITS: entoh_progbits(currScn); break;
    default: break;
    }


  }

  // Successful read !!
  return 0;
}
//-----------------------------------------------------------
static void cleanupMelf(Melf* MelfDesc)
{
  Melf_Scn *currScn, *nextScn;
  currScn = MelfDesc->m_scn_1; 
  while (currScn != NULL){
    nextScn = (Melf_Scn*)currScn->list.next;
    if (NULL != currScn->m_data)
      free(currScn->m_data);
    free(currScn);
    currScn = nextScn;
  }
  return;
}
//-----------------------------------------------------------
static int insertScn(Melf_Scn* scn, Melf* pmelf)
{
  if (((NULL == pmelf->m_scn_1) && (NULL != pmelf->m_scn_n))||
      ((NULL != pmelf->m_scn_1) && (NULL == pmelf->m_scn_n))){
    // Linked list is corrupted
    return -1;
  }

  if ((NULL == pmelf->m_scn_1) && (NULL == pmelf->m_scn_n)){
    // Linked list is empty
    pmelf->m_scn_1 = scn;
    pmelf->m_scn_n = scn;
    scn->list.next = NULL;
    scn->list.prev = NULL;
    return 0;
  }

  // Insert At End
  pmelf->m_scn_n->list.next = (list_t*)scn;
  scn->list.next = NULL;
  scn->list.prev = (list_t*)pmelf->m_scn_n;
  pmelf->m_scn_n = scn;
  return 0;
}
//-----------------------------------------------------------
static inline int scnOrder(Melf_Scn* scn)
{
  switch (scn->m_shdr.sh_type){
  case SHT_STRTAB: return 0;
  case SHT_SYMTAB: return 1;
  case SHT_RELA: return 2;
  case SHT_NULL: return 4;
  default: return 3;
  }
  return 3;
}
//-----------------------------------------------------------
int ltcmpscn(list_t* lhop, list_t* rhop)
{
  Melf_Scn *l, *r;
  int lv, rv;
  l = (Melf_Scn*)lhop;
  r = (Melf_Scn*)rhop;
  lv = scnOrder(l);
  rv = scnOrder(r);
  DEBUG("Compare: %s(%d) %s(%d)\n", SecTypeStrTab[l->m_shdr.sh_type],
	l->m_shdr.sh_id, SecTypeStrTab[r->m_shdr.sh_type], r->m_shdr.sh_id); 
  if (lv < rv)
    return 1;
  return 0;
}
//-----------------------------------------------------------
void swapscn(list_t* x, list_t* y)
{
  Melf_Scn *a, *b;
  Melf_Scn temp;
  a = (Melf_Scn*)x;
  b = (Melf_Scn*)y;
  DEBUG("Swap: %s(%d) %s(%d)\n", SecTypeStrTab[a->m_shdr.sh_type],
	a->m_shdr.sh_id, SecTypeStrTab[b->m_shdr.sh_type], b->m_shdr.sh_id);
  // temp = b
  temp.m_data = b->m_data;
  memcpy(&(temp.m_shdr), &(b->m_shdr), sizeof(Melf_Shdr));
  // b = a
  b->m_data = a->m_data;
  memcpy(&(b->m_shdr), &(a->m_shdr), sizeof(Melf_Shdr));
  // a = temp
  a->m_data = temp.m_data;
  memcpy(&(a->m_shdr), &(temp.m_shdr), sizeof(Melf_Shdr));

#ifdef DBGMODE
  printSecOrder(dbgmelf);
#endif//DBGMODE
  return;
}
//-----------------------------------------------------------
#ifdef DBGMODE
static int printSecOrder(Melf* MelfDesc)
{
  Melf_Scn *currSec;
  for (currSec = MelfDesc->m_scn_1; currSec != NULL; currSec = (Melf_Scn*)currSec->list.next){
    printf("%s(%d) ", SecTypeStrTab[currSec->m_shdr.sh_type], currSec->m_shdr.sh_id);
  }
  printf("\n");
  return 0;
}
#endif // DBGMODE
//-----------------------------------------------------------
// ENDIAN-NESS - ENTOH
//-----------------------------------------------------------
static void entoh_symboltable(Melf_Scn* symscn)
{
  int i;
  Melf_Sym* msym;
  DEBUG("Symbol Table: Network ---> Host\n");
  msym = (Melf_Sym*)symscn->m_data;
  for (i = 0; i < (symscn->m_shdr.sh_size)/(sizeof(Melf_Sym)); i++)
    entoh_Sym(&msym[i]);
  return;
}
//-----------------------------------------------------------
static void entoh_relatable(Melf_Scn* relascn)
{
  int i;
  Melf_Rela* mrela;
  DEBUG("Rela Table: Network ---> Host\n");
  mrela = (Melf_Rela*)relascn->m_data;
  for (i = 0; i < (relascn->m_shdr.sh_size)/(sizeof(Melf_Rela)); i++)
    entoh_Rela(&mrela[i]);
  return;
}
//-----------------------------------------------------------
/*
static void entoh_progbits(Melf_Scn* progscn)
{
  int i;
  Melf_Word* pdata;
  DEBUG("Progbits: Network ---> Host\n");
  pdata = (Melf_Word*)progscn->m_data;
  for (i = 0; i < (progscn->m_shdr.sh_size)/(sizeof(Melf_Word)); i++)
    pdata[i] = entoh_Melf_Word(pdata[i]);
  return;
}
*/
//-----------------------------------------------------------
// ENDIAN-NESS - EHTON
//-----------------------------------------------------------
static Melf_Sym* ehton_symboltable(Melf_Scn* symscn)
{
  int i;
  Melf_Sym* msym;
  DEBUG("Symbol Table: Host ---> Network\n");
  msym = malloc(symscn->m_shdr.sh_size);
  memcpy((void*)msym, symscn->m_data, symscn->m_shdr.sh_size);  
  for (i = 0; i < (symscn->m_shdr.sh_size)/(sizeof(Melf_Sym)); i++)
    ehton_Sym(&msym[i]);
  return msym;
}
//-----------------------------------------------------------
static Melf_Rela* ehton_relatable(Melf_Scn* relascn)
{
  int i;
  Melf_Rela* mrela;
  DEBUG("Rela Table: Host ---> Network\n");
  mrela = malloc(relascn->m_shdr.sh_size);
  memcpy((void*)mrela, relascn->m_data, relascn->m_shdr.sh_size);  
  for (i = 0; i < (relascn->m_shdr.sh_size)/(sizeof(Melf_Rela)); i++)
    ehton_Rela(&mrela[i]);
  return mrela;
}
//-----------------------------------------------------------
/*
static void* ehton_progbits(Melf_Scn* progscn)
{
  int i;
  Melf_Word* pdata;
  DEBUG("Progbits: Host ---> Network\n");
  pdata = malloc(progscn->m_shdr.sh_size);
  memcpy((void*)pdata, progscn->m_data, progscn->m_shdr.sh_size);
  for (i = 0; i < (progscn->m_shdr.sh_size)/(sizeof(Melf_Word)); i++)
    pdata[i] = ehton_Melf_Word(pdata[i]);
  return pdata;
  }*/
//-----------------------------------------------------------
