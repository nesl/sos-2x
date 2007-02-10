/**
 * \file elftomini.c
 * \brief Create a MiniELF format file from a given ELF file
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \author Simon Han {simonhan@cs.ucla.edu}
 */

/**
 * \note Currently the following sections are written to MELF File
 * -# PROGBITS - Code
 * -# RELA     - Relocation
 * -# SYMTAB   - Symbol Table
 */

#define DBGMODE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <soself.h>
#include <minielf.h>
#include <minielflib.h>
#include <elfhelper.h>
#include <sos_endian.h>

#ifdef DBGMODE
#include <dispmelf.h>
#endif


#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...) 
#endif

typedef struct {
  
  Elf32_Sym* esym;
  int numE;
  Melf_Sym* msym;
  int numM;
  unsigned char *strtab;
  unsigned char *rawtext;
  int* etommap;
#ifdef DBGMODE
  int* mtoemap; // For pretty printing
#endif
  Melf_Data* mreladata;      // The relocation table for MELF
  Melf_Data* textdata;       // Text section in MELF
  sos_mod_header_t *sos_mod_hdr_ptr;  // Pointer to module header in MELF text section
  int sos_mod_hdr_offset;             // The offset at the beginning of module header in text section
  int sos_mod_hdr_end;                // The offset to the end of module header in text section
} symbol_map_t; 


static int printusage();
static int convElfToMiniFile(char* elffilename, char* melffilename);
static Melf_Data* convRelaScn(Elf_Scn* relascn, symbol_map_t* symmap); 
static Melf_Data* convSymScn(symbol_map_t* symmap);
static Melf_Data* convProgbitsScn(Elf_Scn* progbitsscn);
static int initSymbolMap(symbol_map_t* symmap, Elf_Scn* symtabscn);
static void addTxtScnToSymbolMap( symbol_map_t* symmap, Elf_Scn* progbitsscn );
static void addStrTabToSymbolMap( symbol_map_t* symmap, Elf_Scn* strtabscn );
static int addMelfSymbol(symbol_map_t* symmap, int elfsymndx);
static int convElfHdr(Melf_Scn* mscn, Elf_Scn* escn);
static void fixUndefinedSymbols( symbol_map_t* symmap );
static void printSOSModHeader( sos_mod_header_t *hdr );
#ifdef DBGMODE
static void  prettyPrintMELF(Melf* melfDesc);
#endif

static int e_machine;
static Elf* elf;

int main(int argc, char** argv)
{
  int ch;
  char *melffilename;

  while ((ch = getopt(argc, argv, "ho:")) != -1){
    switch (ch){
    case 'o': melffilename = optarg; break;
    case 'h': case '?':
      printusage();
      exit(EXIT_FAILURE);
    }
  }

  argc -= optind;
  argv += optind;
  
  if ((NULL != argv[0]) && (NULL != melffilename)) {
    convElfToMiniFile(argv[0], melffilename);
  }
  else{
    printusage();
  }
  return 0;
}


// Initialized in initSymbolMap
//static Elf32_Sym *esym;


//--------------------------------------------------------------------------------
static int convElfToMiniFile(char* elffilename, char* melffilename)
{
  int fdelf, fdmelf, modhdrsymndx;
  Elf32_Ehdr *ehdr;
  Elf_Scn *relatextscn, *textscn, *symtabscn, *strtabscn;
  
  Melf_Scn *m_relatextscn, *m_textscn, *m_symtabscn;
  Melf* melf;
  symbol_map_t* symmap;
  Melf_Data *relatextdata, *symtabdata, *textdata;
  symbol_map_t symmap_storage;

  printf("ELF File: %s\n", elffilename);
  printf("Mini-ELF File: %s\n", melffilename);
  
  symmap = &symmap_storage;
  // Open ELF file for reading
  if ((fdelf = open(elffilename, O_RDONLY)) < 0){
    fprintf(stderr, "%s: ", elffilename);
    perror("convElfToMini -> fopen: ");
    exit(EXIT_FAILURE);
  }

  // Open Mini-ELF file for writing
  if ((fdmelf = open(melffilename, (O_RDWR | O_CREAT | O_EXCL), (S_IRWXU | S_IRWXG | S_IRWXO))) < 0){
    fprintf(stderr, "%s: ", melffilename);
    perror("convElfToMini -> fopen: ");
    exit(EXIT_FAILURE);
  }

  // Check version of ELF Library
  if (elf_version(EV_CURRENT) == EV_NONE){
    fprintf(stderr, "Library version is out of date.\n");
    exit(EXIT_FAILURE);
  }

  // Get the ELF descriptor
  elf = elf_begin(fdelf, ELF_C_READ, NULL);
  if (NULL == elf){
    fprintf(stderr, "elf_begin: Error getting elf descriptor\n");
    exit(EXIT_FAILURE);
  }

  // Ensure that it is an ELF format file
  if (elf_kind(elf) != ELF_K_ELF){
    fprintf(stderr, "This program can only read ELF format files.\n");
    exit(EXIT_FAILURE);
  }

  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }

  // Verify if the ELF is AVR or MSP430 specific  
  if (ehdr->e_machine != EM_AVR && ehdr->e_machine != EM_MSP430){
    fprintf(stderr, "This ELF binary is not supported.\n");
    exit(EXIT_FAILURE);
  }
  e_machine = ehdr->e_machine;
                                                         
  // Create a new Mini-ELF (MELF) descriptor
  if ((melf = melf_new(fdmelf)) == NULL){
    fprintf(stderr, "Error creating a new MELF descriptor\n");
    exit(EXIT_FAILURE);
  }

  // Get the pointer to .symtab section
  if ((symtabscn = getELFSectionByName(elf, ".symtab")) == NULL){
    fprintf(stderr, "Error getting .symtab section.\n");
    exit(EXIT_FAILURE);
  }

  // Get the pointer to .rela.text section
  relatextscn = getELFSectionByName(elf, ".rela.text");

  // Get the pointer to .text section
  if ((textscn = getELFSectionByName(elf, ".text")) == NULL){
    fprintf(stderr, "Error getting .text section.\n");
    exit(EXIT_FAILURE);
  }
  
  if ((strtabscn = getELFSectionByName(elf, ".strtab")) == NULL ) {
    fprintf(stderr, "Error getting .strtab section. \n");
    exit(EXIT_FAILURE);
  }

  // DEBUG info. to verify we have the correct sections
  DEBUG("Section Number (.text): %d\n", (int)elf_ndxscn(textscn));
  if( relatextscn != NULL ) {
    DEBUG("Section Number (.rela.text): %d\n", (int)elf_ndxscn(relatextscn));
  }
  DEBUG("Section Number (.symtab): %d\n", (int)elf_ndxscn(symtabscn));

  // Initialize Symbol Map
  if (initSymbolMap(symmap, symtabscn) < 0){
    fprintf(stderr, "Error initializing symbol map.\n");
    exit(EXIT_FAILURE);
  }
  
  //
  // Set string table to symbol map
  //
  addStrTabToSymbolMap( symmap, strtabscn );
  addTxtScnToSymbolMap( symmap, textscn );
  

  // Convert Relocation Table
  if( relatextscn != NULL ) {
    if ((relatextdata = convRelaScn(relatextscn, symmap)) == NULL){
      fprintf(stderr, "Error converting Rela table.\n");
      exit(EXIT_FAILURE);
    }
  }

  // Convert Text Section
  if ((textdata = convProgbitsScn(textscn)) == NULL){
    fprintf(stderr, "Error converting text section.\n");
    exit(EXIT_FAILURE);
  }
  symmap->textdata = textdata;

  // Add mod_header symbol
  if ((modhdrsymndx = getELFSymbolTableNdx(elf, "mod_header")) == -1){
    fprintf(stderr, "Could not locate symbol: mod_header. Not a valid SOS module\n");
    exit(EXIT_FAILURE);
  }
  
  addMelfSymbol(symmap, modhdrsymndx);
  melf_setModHdrSymNdx(melf, symmap->etommap[modhdrsymndx]);

  printf("MELF module header index = %d\n", symmap->etommap[modhdrsymndx] );
  {
    Melf_Sym *modhdrsym = &(symmap->msym[ symmap->etommap[modhdrsymndx] ]);
    symmap->sos_mod_hdr_ptr = (sos_mod_header_t*)(((char*)(textdata->d_buf)) + modhdrsym->st_value);
    symmap->sos_mod_hdr_offset = modhdrsym->st_value;
    symmap->sos_mod_hdr_end = modhdrsym->st_value + sizeof(sos_mod_header_t) + 
      (sizeof(sos_func_cb_t) * 
       (symmap->sos_mod_hdr_ptr->num_sub_func + symmap->sos_mod_hdr_ptr->num_prov_func + symmap->sos_mod_hdr_ptr->num_dfunc));
		
    //printf("MELF module header Start: %4d\n", symmap->sos_mod_hdr_offset);
    //printf("MELF module header End: %4d\n", symmap->sos_mod_hdr_end);
    printSOSModHeader( symmap->sos_mod_hdr_ptr );
    fixUndefinedSymbols( symmap );
  }
  
  // Convert Symbol Table
  if ((symtabdata = convSymScn(symmap)) == NULL){
    fprintf(stderr, "Error convering symbol table.\n");
    exit(EXIT_FAILURE);
  }
  
  // Create a new MELF Symbol table section
  if ((m_symtabscn = melf_new_scn(melf)) == NULL){
    fprintf(stderr, "Error creating symbol table MELF section.\n");
    exit(EXIT_FAILURE);
  }
  convElfHdr(m_symtabscn, symtabscn);
  melf_add_data_to_scn(m_symtabscn, symtabdata);

  // Create a new MELF Relocation table section
  if( relatextscn != NULL ) {
    if ((m_relatextscn = melf_new_scn(melf)) == NULL){
      fprintf(stderr, "Error creating relocation table MELF section.\n");
      exit(EXIT_FAILURE);
    }
    convElfHdr(m_relatextscn, relatextscn);
    melf_add_data_to_scn(m_relatextscn, relatextdata);
  }

  // Create a new MELF Symbol table section
  if ((m_textscn = melf_new_scn(melf)) == NULL){
    fprintf(stderr, "Error creating .text MELF section.\n");
    exit(EXIT_FAILURE);
  }
  convElfHdr(m_textscn, textscn);
  melf_add_data_to_scn(m_textscn, textdata);
  
  // Sort the MELF Sections (Enable single pass writing on the nodes)
  melf_sortScn(melf);

  // Set the section offsets
  melf_setScnOffsets(melf);

  // Write the MELF File
  melf_write(melf);

#ifdef DBGMODE
  prettyPrintMELF(melf);
#endif

  // Close ELF Descriptor
  elf_end(elf);
  // Close ELF File
  close(fdelf);
  // Close MELF File
  close(fdmelf);
  return 0;
}
//---------------------------------------------------------------------
static int convElfHdr(Melf_Scn* mscn, Elf_Scn* escn)
{
  Elf32_Shdr *shdr; 
  if ((shdr = elf32_getshdr(escn)) == NULL){
    fprintf(stderr, "Error reading section header.\n");
    return -1;
  } 
  mscn->m_shdr.sh_id = (Melf_Half)elf_ndxscn(escn);
  mscn->m_shdr.sh_type = shdr->sh_type;
  mscn->m_shdr.sh_link = shdr->sh_link;
  mscn->m_shdr.sh_info = shdr->sh_info;
  return 0;
}


//---------------------------------------------------------------------
static Melf_Data* convProgbitsScn(Elf_Scn* progbitsscn)
{
  Elf32_Shdr *shdr;
  Elf_Data   *edata;
  Melf_Data  *mdata;

  if ((shdr = elf32_getshdr(progbitsscn)) == NULL){
    fprintf(stderr, "Error reading progbits section header.\n");
    return NULL;
  }

  if (SHT_PROGBITS != shdr->sh_type){
    fprintf(stderr, "Not a progbits section.\n");
    return NULL;
  }

  edata = NULL;
  // Get the RAW data from the ELF File for the Progbits section
  while ((edata = elf_rawdata(progbitsscn, edata)) != NULL){
    if (ELF_T_BYTE == edata->d_type){
      // Allocate memory for mdata
      if ((mdata = malloc(sizeof(Melf_Data))) == NULL){
	fprintf(stderr, "Could not allocte memory for Mini-ELF progbits mdata structure.\n");
	return NULL;
      }
      if ((mdata->d_buf = malloc(edata->d_size)) == NULL){
	fprintf(stderr, "Could not allocate memory for Mini-ELF progbits data.\n");
	return NULL;
      }
      mdata->d_type = ELF_T_BYTE;
      memcpy(mdata->d_buf, edata->d_buf, edata->d_size);
      mdata->d_size = edata->d_size;
      mdata->d_numData = edata->d_size;
      return mdata;
    }
  }
  fprintf(stderr, "No data in the progbits section.\n");
  return NULL;
}
//---------------------------------------------------------------------
static int initSymbolMap(symbol_map_t* symmap, Elf_Scn* symtabscn)
{
  Elf32_Shdr *shdr;
  Elf_Data *edata;

  if ((shdr = elf32_getshdr(symtabscn)) == NULL){
    fprintf(stderr, "Error reading symbol table section header.\n");
    return -1;
  }

  if (SHT_SYMTAB != shdr->sh_type){
    fprintf(stderr, "Not a symbol table section.\n");
    return -1;
  }

  edata = NULL;
  while ((edata = elf_getdata(symtabscn, edata)) != NULL){
    if (ELF_T_SYM == edata->d_type){
      int i;
      symmap->esym = (Elf32_Sym*)edata->d_buf;
      //esym = (Elf32_Sym*)edata->d_buf;
      symmap->numE = (int)(edata->d_size/ shdr->sh_entsize);
      if ((symmap->msym = malloc(sizeof(Melf_Sym)* symmap->numE)) == NULL){
	fprintf(stderr, "Error allocating memory.\n");
	return -1;
      }
      if ((symmap->etommap = malloc(sizeof(int)* symmap->numE)) == NULL){
	fprintf(stderr, "Error allocating memory.\n");
	return -1;
      }
#ifdef DBGMODE
      if ((symmap->mtoemap = malloc(sizeof(int)* symmap->numE)) == NULL){
	fprintf(stderr, "Error allocating memory.\n");
	return -1;
      }
      for (i = 0; i < symmap->numE; i++)
	symmap->mtoemap[i] = -1;
#endif
      symmap->numM = 0;
      for (i = 0; i < symmap->numE; i++)
	symmap->etommap[i] = -1;
      return 0;
    }
  }
  fprintf(stderr, "Section does not contain any symbols.\n");
  return -1;
}

//---------------------------------------------------------------------
static void addStrTabToSymbolMap( symbol_map_t* symmap, Elf_Scn* strtabscn )
{
  Elf_Data *edata = NULL;
	
  while ((edata = elf_getdata(strtabscn, edata)) != NULL) {
    if (ELF_T_BYTE == edata->d_type){
      symmap->strtab = (unsigned char*) edata->d_buf;
      return;
    }
  }
  fprintf(stderr, "Section does not contain any strings.\n");
  exit(EXIT_FAILURE);
}

static void addTxtScnToSymbolMap( symbol_map_t* symmap, Elf_Scn* progbitsscn )
{
  Elf_Data *edata = NULL;
  while ((edata = elf_rawdata(progbitsscn, edata)) != NULL){
    if (ELF_T_BYTE == edata->d_type){
      symmap->rawtext = (unsigned char*) edata->d_buf;
      return;
    }
  }
  fprintf(stderr, "Section does not contain any bytes.\n");
  exit(EXIT_FAILURE);
}

//---------------------------------------------------------------------
static Melf_Data* convSymScn(symbol_map_t* symmap)
{
  Melf_Data* msymdata;
  // Allocate memory
  if ((msymdata = malloc(sizeof(Melf_Data))) == NULL){
    fprintf(stderr, "Error allocating memory.");
    return NULL;
  }
  msymdata->d_buf = symmap->msym;
  msymdata->d_numData = symmap->numM;
  msymdata->d_type = ELF_T_SYM;
  msymdata->d_size = symmap->numM * sizeof(Melf_Sym);
  return msymdata;
}

//---------------------------------------------------------------------
static Melf_Data* convRelaScn(Elf_Scn* relascn, symbol_map_t* symmap)
{
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Rela *erela;
  Melf_Rela  *mrela;
  int numRecs, i;
  Melf_Data* mreladata;

  // Get the ELF Rela Section Header
  if ((shdr = elf32_getshdr(relascn)) == NULL){
    fprintf(stderr, "Error reading rela section header.\n");
    return NULL;
  }
  // Verify the ELF RELA section type
  if (SHT_RELA != shdr->sh_type){
    fprintf(stderr, "Not a relocation section. \n");
    return NULL;
  }
  // Get the ELF rela table
  edata = NULL;
  while ((edata = elf_getdata(relascn, edata)) != NULL){
    if (ELF_T_RELA == edata->d_type){
      erela = (Elf32_Rela*)edata->d_buf;
      numRecs = edata->d_size/shdr->sh_entsize;
      // Allocating memory for MELF rela table
      if ((mreladata = malloc(sizeof(Melf_Data))) == NULL){
	fprintf(stderr, "Error allocating memory.");
	return NULL;
      }
      if ((mreladata->d_buf = malloc(sizeof(Melf_Rela) * numRecs)) == NULL){
	fprintf(stderr, "Error allocating memory.");
	return NULL;
      }
      // Init the MELF rela table
      mreladata->d_numData = 0;      
      mreladata->d_type = ELF_T_RELA;
      mrela = (Melf_Rela*)mreladata->d_buf;
      for (i = 0; i < numRecs; i++){
	int esymndx;
	int msymndx;
	esymndx = ELF32_R_SYM(erela[i].r_info);
		
	// Get MELF Symbol Index
	if ((msymndx = addMelfSymbol(symmap, esymndx)) == -1){
	  fprintf(stderr, "Invalid symbol index in rela.\n");
	  return NULL;
	}
	
	// Convert ELF Rela record to MELF Rela	record
	mrela[mreladata->d_numData].r_offset = (Melf_Addr)erela[i].r_offset;
	mrela[mreladata->d_numData].r_symbol = msymndx;
	mrela[mreladata->d_numData].r_type = (unsigned char)ELF32_R_TYPE(erela[i].r_info);
	mrela[mreladata->d_numData].r_addend = (Melf_Sword) erela[i].r_addend;
	//
	mreladata->d_numData++;
      }
      mreladata->d_size = mreladata->d_numData * sizeof(Melf_Rela);
      symmap->mreladata = mreladata;
      return mreladata;
    }
  }
  symmap->mreladata = NULL;
  fprintf(stderr, "Section does not contain any rela records.\n");
  return NULL;
}

//---------------------------------------------------------------------
static int addMelfSymbol(symbol_map_t* symmap, int elfsymndx)
{
  Melf_Sym* msym;
  Elf32_Sym* esym;
  int idx_to_string;

  // Check if the ELF symbol index is valid
  if (elfsymndx > symmap->numE){
    fprintf(stderr,"addMelfSymbol: Invalid ELF Symbol index.\n");
    return -1;
  }
  // Check if this ELF symbol is already present in MELF table
  if (symmap->etommap[elfsymndx] == -1){
    // Set the symbol table maps
    symmap->etommap[elfsymndx] = symmap->numM;
#ifdef DBGMODE
    symmap->mtoemap[symmap->numM] = elfsymndx;
#endif
    idx_to_string = (symmap->esym[elfsymndx]).st_name;
    if( ELF32_ST_TYPE((symmap->esym[elfsymndx]).st_info) == STT_SECTION ) {
      char *sh_name = getELFSectionName(elf,
					(symmap->esym[elfsymndx]).st_shndx);
      printf("Add ELF Symbol to MELF: %s + %d\n",
	     sh_name,    
	     (symmap->esym[elfsymndx]).st_value);
    } else {
      printf("Add ELF Symbol to MELF: %s\n", &(symmap->strtab[idx_to_string]));
    }
    // Add this ELF symbol to the MELF symbol table
    esym = &(symmap->esym[elfsymndx]);
    msym = &(symmap->msym[symmap->numM]);
    
    msym->st_value = (Melf_Addr)esym->st_value;
    msym->st_info = esym->st_info;
    msym->st_shid = (Melf_Half)esym->st_shndx;
    
    symmap->numM++;
  }
  return (symmap->etommap[elfsymndx]);
}

/*
  static void printELFSymbol(symbol_map_t* symmap, int elfsymndx )
  {
  Elf32_Sym* esym;
  //Elf_Data* edata;
  printf("printELFSymbol\n");
  esym = &(symmap->esym[elfsymndx]);
	
  printf("Value %08x ", esym->st_value);
  printf("Size %6d ", esym->st_size);
  switch(esym->st_shndx) {
  case SHN_ABS: printf("ABS "); break;                  
  case SHN_COMMON: printf("CMN "); break;               
  case SHN_UNDEF: printf("UND "); break;                
  default: printf("%3d ", esym->st_shndx); break;     
  }
	
  printf("\n");
  }
*/

static void fixUndefinedSymbols( symbol_map_t* symmap )
{
  int i;
  Melf_Rela  *mrela;
	
  if( symmap->mreladata == NULL ) {
    fprintf(stderr, "Invalid relocation data!\n");
    exit(1);
  }
  mrela = (Melf_Rela*)(symmap->mreladata->d_buf);
  //
  // Search through each relocation record
  // If the record points to undinfed symbol and the record points to SOS function table,
  // Store module ID and function ID to st_value
  //
  for( i = 0; i < symmap->mreladata->d_numData; i++ ) {
    Melf_Sym *msym = &(symmap->msym[ mrela[i].r_symbol ]);
		
    if( (MELF_ST_BIND( msym->st_info ) == STB_GLOBAL) && 
	(MELF_ST_TYPE( msym->st_info ) == STT_NOTYPE) &&
	(SHN_UNDEF == msym->st_shid )) {
      //printf("Relocation Record contains undefined symbol, Offset = %4x\n", mrela[i].r_offset);
			
      if( mrela[i].r_offset >= (symmap->sos_mod_hdr_offset + offsetof(sos_mod_header_t, funct)) && 
	  mrela[i].r_offset < symmap->sos_mod_hdr_end ) {
	int func_idx = mrela[i].r_offset - symmap->sos_mod_hdr_offset - offsetof(sos_mod_header_t, funct); 
				
	//printf("In module header %d\n", func_idx);
	if( func_idx % sizeof(sos_func_cb_t) == 0 ) {
	  func_idx /= sizeof(sos_func_cb_t);
	  //printf("func_idx = %d\n", func_idx);
	  // Change STT_NOTYPE to STT_SOS_DFUNC
	  msym->st_info = MELF_ST_INFO( STB_GLOBAL, STT_SOS_DFUNC );
	  msym->st_value = ((Melf_Addr)(symmap->sos_mod_hdr_ptr->funct[func_idx].pid) << 8)
	    | (Melf_Addr)(symmap->sos_mod_hdr_ptr->funct[func_idx].fid);
	} else {
	  //printf("func_idx is not pointing to correct location\n");
	  exit(1);
	}
      } else {
	//printf("NOT in module header\n");
      }
    }
  }
	
  //
  // If we still have undefined symbol, fire Error!
  //
  for( i = 0; i < symmap->numM; i++ ) {
    Melf_Sym *msym = &(symmap->msym[i]);
	  
    if( (MELF_ST_BIND( msym->st_info ) == STB_GLOBAL) && 
	(MELF_ST_TYPE( msym->st_info ) == STT_NOTYPE) &&
	(SHN_UNDEF == msym->st_shid )) {
      // If this relocation record is still undefined
      // TODO: print error information
      int elfsymndx = symmap->mtoemap[i];
      Elf32_Sym *esym = &(symmap->esym[elfsymndx]);
	    
      printf("ERROR: Undefined symbol: %s\n", &(symmap->strtab[esym->st_name]));
      exit(1);
    } 
  }

	
}

static void printSOSModHeader( sos_mod_header_t *hdr )
{
  int i;
  printf("====== SOS Module Header =====\n");
  printf("Module ID:          %4d\n", hdr->mod_id);
  printf("State Size:         %4d\n", entohs(hdr->state_size));
  printf("Number Sub Func:    %4d\n", hdr->num_sub_func);
  printf("Number Prov Func:   %4d\n", hdr->num_prov_func);
  printf("Number Direct Func: %4d\n", hdr->num_dfunc); 
  printf("Processor Type:     %4d\n", hdr->processor_type);
  printf("Platform Type:      %4d\n", hdr->platform_type);
  for( i = 0; i < hdr->num_sub_func + hdr->num_prov_func; i++ ) {
    printf("Funcion %d: ADDR: %4x, PID: %4d, FID: %4d\n", i, hdr->funct[i].ptr, hdr->funct[i].pid, hdr->funct[i].fid);
  }
	
  printf("\n");
}

//---------------------------------------------------------------------
static int printusage()
{
  printf("ELF to MINI-ELF converter\n");
  printf("elftomini [-h] -o <MELF-filename> elf-filename\n");
  return 0;
}
//---------------------------------------------------------------------
#ifdef DBGMODE
static void  prettyPrintMELF(Melf* melfDesc)
{
  PrintMelfHdr(melfDesc);
  PrintMelfSecTable(melfDesc);
  PrintMelfSymTable(melfDesc);
  PrintMelfRelaTable(melfDesc);
  return;
}
#endif//DBGMODE
