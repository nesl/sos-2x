/**
 * \file avr_file_write.c
 * \brief Routines to write sandbox binary to file
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <soself.h>
#include <basicblock.h>
#include <fileutils.h>
#include <elfhelper.h>
#include <elfdisplay.h>
#include <avr_minielf.h>

#include <avrsandbox.h>

#include <sos_mod_header_patch.h>

//-------------------------------------------------------------------
// STATIC FUNCTIONS
static void avr_create_new_elf_header(Elf32_Ehdr *ehdr, Elf32_Ehdr *nehdr);
static void avr_create_new_section_header(Elf32_Shdr *shdr, Elf32_Shdr *nshdr);
static void avr_create_new_data(Elf_Data *edata, Elf_Data *nedata);
static void avr_create_new_text_data(Elf_Data* edata, Elf_Data* nedata, 
				     file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr);
static void avr_create_new_rela_text_data(Elf_Data* nedata, Elf* elf,
					  Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist);
static void avr_create_new_symbol_table(Elf_Data* nedata, Elf* elf, Elf* nelf,
					Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist);
//-------------------------------------------------------------------
void avr_write_binfile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr)
{
  FILE *outbinFile;
  basicblk_t* cblk;
  int i;
  avr_instr_t instr;
  uint8_t* buff;

  /*
  inbinFile = fopen(inFileName, "r");
  if (NULL == inbinFile){
    fprintf(stderr, "Error opening file %s\n", inFileName);
    exit(EXIT_FAILURE);
  }  
  */
  if (fdesc->type != BIN_FILE) return;
  
  outbinFile = fopen(outFileName, "w");
  if (NULL == outbinFile){
    fprintf(stderr, "Error opening file %s\n", outFileName);
    exit(EXIT_FAILURE);
  }  
  
  if ((buff = malloc(sizeof(uint8_t)*startaddr)) == NULL){
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }

  obj_file_seek(fdesc, 0, SEEK_SET);
  if (obj_file_read(fdesc, buff, sizeof(uint8_t), startaddr) == 0){
    fprintf(stderr, "avr_write_binfile: Premature end of input file.\n");
    exit(EXIT_FAILURE);
  }

  // Patch the SOS module header
  sos_patch_mod_header(blist, buff);

  if (fwrite(buff, sizeof(uint8_t), startaddr, outbinFile) == 0){
      fprintf(stderr, "avr_write_binfile: Premature end of output file.\n");
      exit(EXIT_FAILURE);
  }

  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    for (i = 0; i < (int)(cblk->newsize/sizeof(avr_instr_t)); i++){
      instr.rawVal = cblk->newinstr[i].rawVal;
#ifdef BBIG_ENDIAN
      instr.rawVal = Flip_int16(instr.rawVal);
#endif
      if (fwrite(&instr, sizeof(avr_instr_t),1, outbinFile) != 1){
	fprintf(stderr, "avr_write_binfile: Error during file write.\n");
	exit(EXIT_FAILURE);
      }
    }    
  }  
  fclose(outbinFile);
  return;
}
//-------------------------------------------------------------------
void avr_write_elffile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr)
{
  int fd;
  Elf *elf, *nelf;
  Elf32_Ehdr *ehdr, *nehdr;
  Elf_Scn *scn, *nscn;
  Elf32_Shdr *shdr, *nshdr;
  Elf_Data *edata, *nedata;

  DEBUG("=========== ELF File Write ===========\n");

  elf = fdesc->elf;
  
  fd = open(outFileName, O_RDWR|O_TRUNC|O_CREAT, 0666);
  if ((nelf = elf_begin(fd, ELF_C_WRITE, (Elf *)NULL)) == 0){
    fprintf(stderr, "avr_write_elffile: Error creating output ELF archive.\n");
    exit(EXIT_FAILURE);
  }
  
  if ((nehdr = elf32_newehdr(nelf)) == NULL){
    fprintf(stderr, "avr_write_elffile: Error creating new ELF header.\n");
    exit(EXIT_FAILURE);
  }
  if ((ehdr = elf32_getehdr(elf)) == NULL){
    fprintf(stderr, "avr_write_elfffile: Error reading ELF header.\n");
    exit(EXIT_FAILURE);
  }
  avr_create_new_elf_header(ehdr, nehdr);
  
  
  scn = NULL;
  while ((scn = elf_nextscn(fdesc->elf, scn)) != NULL){
    nscn = elf_newscn(nelf);
    nshdr = elf32_getshdr(nscn);
    shdr = elf32_getshdr(scn);
    avr_create_new_section_header(shdr, nshdr);
    edata = NULL;
    edata = elf_getdata(scn, edata);
    nedata = elf_newdata(nscn);
    
    // Get name of current section
    char* CurrSecName = NULL;
    CurrSecName = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
    // Compare with .text section
    if (strcmp(CurrSecName, ".text") == 0)
      avr_create_new_text_data(edata, nedata, fdesc, blist, startaddr);
    // Comare with .rela.text section
    else if (strcmp(CurrSecName, ".rela.text") == 0){
      avr_create_new_data(edata, nedata);
      avr_create_new_rela_text_data(nedata, elf, nshdr, startaddr, blist);
    }
    else if (strcmp(CurrSecName, ".symtab") == 0){
      avr_create_new_data(edata, nedata);
      avr_create_new_symbol_table(nedata, elf, nelf, nshdr, startaddr, blist);
    }
    else
      avr_create_new_data(edata, nedata);
    elf_update(nelf, ELF_C_WRITE);
  }
  elf_end(nelf);
}
//-------------------------------------------------------------------
static void avr_create_new_symbol_table(Elf_Data* nedata, Elf* elf, Elf* nelf,
					Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist)
{
  Elf32_Ehdr* ehdr;
  Elf32_Sym* nsym;
  Elf_Data* nreladata;
  Elf32_Rela *nerela;
  int numSyms, numRecs, i, txtscnndx, btxtscnfound;
  Elf_Scn *txtscn, *nrelascn;
  Elf32_Shdr *txtshdr, *nrelashdr;

  DEBUG("Determine .text section index ...\n");
  // Get the ELF Header
  if ((ehdr = elf32_getehdr(elf)) == NULL){
    fprintf(stderr, "avr_create_new_symbol_table: Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }  
  txtscn = NULL;
  txtscnndx = 1;
  btxtscnfound = 0;
  while ((txtscn = elf_nextscn(elf, txtscn)) != NULL){
    if ((txtshdr = elf32_getshdr(txtscn)) != NULL){
      char* CurrSecName = NULL;
      CurrSecName = elf_strptr(elf, ehdr->e_shstrndx, txtshdr->sh_name);
      if (strcmp(CurrSecName, ".text") == 0){
	btxtscnfound = 1;
	break;
      }
    }
    txtscnndx++;
  }
  if (0 == btxtscnfound){
    fprintf(stderr, "avr_create_new_symbol_table: Cannot find .text section.\n");
    exit(EXIT_FAILURE);
  }
  DEBUG(".text section index: %d\n", txtscnndx);
  
  // Get .rela.text section
  nrelascn = getELFSectionByName(nelf, ".rela.text");
  nrelashdr = elf32_getshdr(nrelascn);
  nreladata = NULL;
  nreladata = elf_getdata(nrelascn, nreladata);
  numRecs = nreladata->d_size/nrelashdr->sh_entsize;

  numSyms = nedata->d_size/nshdr->sh_entsize;
  nsym = (Elf32_Sym*)(nedata->d_buf);
  for (i = 0; i < numSyms; i++){
    if ((nsym->st_shndx == txtscnndx) && (ELF32_ST_TYPE(nsym->st_info) != STT_SECTION)){
      if (nsym->st_value > startaddr){
	uint32_t oldValue = nsym->st_value;
	nsym->st_value = find_updated_address(blist, oldValue);
	// Check if we have to further modify this symbol (if it is used as a call target value)
	int j;
	nerela = (Elf32_Rela*)nreladata->d_buf;
	for (j = 0; j < numRecs; j++){
	  if ((ELF32_R_SYM(nerela->r_info) == i) && (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL)){
	    // Check if it is indeed a call instruction before changing the symbol
	    avr_instr_t instr;
	    instr = find_instr_at_new_addr(blist, nerela->r_offset);
	    if ((instr.rawVal & OP_TYPE10_MASK) == OP_CALL){
	      nsym->st_value -= sizeof(avr_instr_t) * 2;
	      DEBUG("Call target symbol. Modify to accomodate safe stack store.\n");
	    }
	    else
	      DEBUG("Jmp target symbol. No modification to symbol required.\n");
	    break;
	  }
	  // Follwing is only for producing a more readable elf.lst
	  if ((ELF32_ST_BIND(nsym->st_info) == STB_LOCAL) &&
	      (ELF32_ST_TYPE(nsym->st_info) == STT_FUNC) &&
	      (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL) &&
	      (nerela->r_addend == (nsym->st_value - (2*sizeof(avr_instr_t))))){
	    nsym->st_value -= sizeof(avr_instr_t) * 2;
	    DEBUG("Call target symbol. Modified for ELF pretty print.\n");
	  }
	  nerela++;
	}
	DEBUG("Entry: %d Old Value: 0x%x New Value: 0x%x\n", i, (int)oldValue, (int)nsym->st_value);
      }
    }
    nsym++;
  }
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_rela_text_data(Elf_Data* nedata, Elf* elf,
					  Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist)
{
  Elf_Scn* symscn;
  Elf32_Shdr* symshdr;
  Elf_Data *symdata;
  Elf32_Sym* sym;
  Elf32_Rela *nerela;
  int numRecs, numSyms, i, textNdx;
  
  symscn = getELFSymbolTableScn(elf);
  symshdr = elf32_getshdr(symscn);
  symdata = NULL;
  symdata = elf_getdata(symscn, symdata);
  sym = (Elf32_Sym*)symdata->d_buf;
  numSyms = symdata->d_size/symshdr->sh_entsize;
  textNdx = -1;
  DEBUG("Locating symbol table index for .text\n");
  DEBUG("Rela Info: %d\n", (int)nshdr->sh_info);
  for (i = 0; i < numSyms; i++){
    #ifdef DBGMODE
    ElfPrintSymbol(sym);
    DEBUG("\n");
    #endif
    if ((sym->st_shndx == nshdr->sh_info) && (ELF32_ST_TYPE(sym->st_info) == STT_SECTION)){
      textNdx = i;
      break;
    }
    sym++;
  }
  if (-1 == textNdx){
    fprintf(stderr, "avr_create_new_rela_text_data: Could not locate .text section symbol\n");
    exit(EXIT_FAILURE);
  }
  DEBUG(".text symbol table index: %d\n", textNdx);

  DEBUG("Fixing .rela.text entries ...\n");
  nerela = (Elf32_Rela*)nedata->d_buf;
  numRecs = nedata->d_size/nshdr->sh_entsize;
  
  for (i = 0; i < numRecs; i++){
    // Change offset field for all records .rela.text
    // Change addend if symbol field is .text
    if (nerela->r_offset > startaddr){
      uint32_t oldaddr = nerela->r_offset;
      nerela->r_offset = find_updated_address(blist, oldaddr);
      DEBUG("Entry: %d Old Offset: 0x%x New Offset: 0x%x\n", i, (int)oldaddr, (int)nerela->r_offset);
    }
    if (ELF32_R_SYM(nerela->r_info) == textNdx){
      if (nerela->r_addend > startaddr){
	uint32_t old_addend = nerela->r_addend;
	nerela->r_addend = find_updated_address(blist, old_addend);
	// If relocation type is of R_AVR_CALL, then modify addend.
	// The addend should point to two words before the actual function 
	// so as to invoke the call to the safe stack save function
	if (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL){
	  // Check if it is indeed a call instruction before changing the addend
	  avr_instr_t instr;
	  instr = find_instr_at_new_addr(blist, nerela->r_offset);
	  if ((instr.rawVal & OP_TYPE10_MASK) == OP_CALL){
	    nerela->r_addend -= sizeof(avr_instr_t) * 2;
	    DEBUG("Internal call target -> Modify addend to include safe stack.\n");
	  }
	  else
	    DEBUG("Internal jmp target -> No further modifications.\n");
	}
	DEBUG("Entry: %d Old Addend: 0x%x New Addend: 0x%x\n", i, (int)old_addend, nerela->r_addend);
      }
    }
    nerela++;
  }
  
  // Add new relocation records
  basicblk_t* cblk;
  Elf32_Rela *newrela;
  int numnewrecs;
  int newcalljmpflag;
  newrela = (Elf32_Rela*)malloc(sizeof(Elf32_Rela) * MAX_NEW_RELA);
  numnewrecs = 0;
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    avr_instr_t* instr;
    int numinstr;
    uint32_t thisinstroffset;
    numinstr = cblk->newsize/sizeof(avr_instr_t);
    if (NULL == cblk->branch) continue;
    if ((cblk->flag & TWO_WORD_INSTR_FLAG) == 0) continue;
    instr = &(cblk->newinstr[numinstr - 2]);
    thisinstroffset = cblk->newaddr + cblk->newsize - 2 * sizeof(avr_instr_t);
    

    if (((instr->rawVal & OP_TYPE10_MASK) != OP_JMP) &&
	((instr->rawVal & OP_TYPE10_MASK) != OP_CALL)){
      fprintf(stderr, "avr_create_new_rela_text_data: Basic block flag corrupted\n");
      exit(EXIT_FAILURE);
    }


    // Check if there is a rela record with the current offset
    nerela = (Elf32_Rela*)nedata->d_buf;
    newcalljmpflag = 1;
    for (i = 0; i < numRecs; i++){
      if (nerela->r_offset == thisinstroffset){
	DEBUG("Curr New Offset: 0x%d -- Found\n", thisinstroffset);
	newcalljmpflag = 0;
	break;
      }
      nerela++;
    }
    
    if (newcalljmpflag == 0) continue;



    // Add a new relocation records
    // r_added = .text + branch target address
    newrela[numnewrecs].r_info = ELF32_R_INFO(textNdx, R_AVR_CALL);  
    newrela[numnewrecs].r_offset = thisinstroffset;
    newrela[numnewrecs].r_addend = (cblk->branch)->newaddr;
    DEBUG("New Rela: Offset 0x%x  Addend 0x%x\n", newrela[numnewrecs].r_offset, newrela[numnewrecs].r_addend);
    numnewrecs++;
  }

  DEBUG("New Rela Recs: %d\n", numnewrecs);
  
  uint8_t *new_d_buf, *ptr_d_buf;
  new_d_buf = (uint8_t*)malloc(numnewrecs * sizeof(Elf32_Rela) + nedata->d_size);
  memcpy(new_d_buf, nedata->d_buf, nedata->d_size);
  ptr_d_buf = new_d_buf + nedata->d_size;
  memcpy(ptr_d_buf, newrela, sizeof(Elf32_Rela)*numnewrecs);
  free(nedata->d_buf);
  free(newrela);
  nedata->d_buf = new_d_buf;
  nedata->d_size += sizeof(Elf32_Rela) * numnewrecs;
  

  return;
}
//-------------------------------------------------------------------
static void avr_create_new_text_data(Elf_Data* edata, Elf_Data* nedata, 
				     file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr)
{
  basicblk_t* cblk;
  int i;
  avr_instr_t instr;
  avr_instr_t* wrptr;
    
  nedata->d_type = edata->d_type;
  nedata->d_align = edata->d_align;
  nedata->d_size = startaddr;

  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next)
    nedata->d_size += cblk->newsize;
  nedata->d_buf = (uint8_t*)malloc(nedata->d_size);

  obj_file_seek(fdesc, 0, SEEK_SET);
  if (obj_file_read(fdesc, nedata->d_buf, sizeof(uint8_t), startaddr) == 0){
    fprintf(stderr, "avr_write_binfile: Premature end of input file.\n");
    exit(EXIT_FAILURE);
  }
  wrptr = (avr_instr_t*)((uint8_t*)((uint8_t*)nedata->d_buf + startaddr));
 
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    for (i = 0; i < (int)(cblk->newsize/sizeof(avr_instr_t)); i++){
      instr.rawVal = cblk->newinstr[i].rawVal;
#ifdef BBIG_ENDIAN
      instr.rawVal = Flip_int16(instr.rawVal);
#endif
      if (((uint8_t*)wrptr - (uint8_t*)(nedata->d_buf) + sizeof(avr_instr_t)) <= nedata->d_size){
	wrptr->rawVal = instr.rawVal;
	wrptr++;
      }
      else
	fprintf(stderr, "avr_create_new_text_data: Buffer overflow while writing sandbox .text section.\n");
    }    
  }  
  return;
}

//-------------------------------------------------------------------
static void avr_create_new_elf_header(Elf32_Ehdr *ehdr, Elf32_Ehdr *nehdr)
{
  nehdr->e_ident[EI_CLASS] = ehdr->e_ident[EI_CLASS];
  nehdr->e_ident[EI_DATA] = ehdr->e_ident[EI_DATA];
  nehdr->e_type = ehdr->e_type;
  nehdr->e_machine = ehdr->e_machine;
  nehdr->e_flags = ehdr->e_flags;
  nehdr->e_shstrndx = ehdr->e_shstrndx;
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_section_header(Elf32_Shdr *shdr, Elf32_Shdr *nshdr)
{
  nshdr->sh_name = shdr->sh_name;
  nshdr->sh_type = shdr->sh_type;
  nshdr->sh_flags = shdr->sh_flags;
  nshdr->sh_addr = shdr->sh_addr;
  nshdr->sh_size = shdr->sh_size;
  nshdr->sh_link = shdr->sh_link;
  nshdr->sh_info = shdr->sh_info;
  nshdr->sh_addralign = shdr->sh_addralign;
  nshdr->sh_entsize = shdr->sh_entsize;
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_data(Elf_Data *edata, Elf_Data *nedata)
{
  nedata->d_type = edata->d_type;
  nedata->d_size = edata->d_size;
  nedata->d_align = edata->d_align;
  if (edata->d_size > 0){
    nedata->d_buf = malloc(edata->d_size);
    memcpy(nedata->d_buf, edata->d_buf, edata->d_size);
  }
  return;
}
//-------------------------------------------------------------------
