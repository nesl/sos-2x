/**
 * \file sfi_jumptable.c
 * \brief Managing the SFI Jumptable for the modules
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <sfi_jumptable.h>
#include <sfi_exception.h>
#include <sos_module_types.h>
#include <malloc.h>
#include <avrinstr.h>
#include <flash.h>
#include <hardware.h>
#include <cross_domain_cf.h>
#include <memmap.h>

#define CONV_DOMAINID_TO_PAGENUM(x) (uint16_t)(SFI_DOM0_TABLE + (uint16_t)x)
#define CONV_DOMAINID_TO_ADDRESS(x) (uint32_t)((SFI_DOM0_TABLE + (uint32_t)x) * 256)
#define SFI_MOD_TABLE_ENTRY_LOC(page, fnidx) ((page * WORDS_PER_PAGE) + (SIZE_OF_JMP_INSTR * fnidx))

extern char __heap_start;

static void sfi_err_code_led_display(uint8_t errcode);

typedef struct _sfi_jmptblrec{
  sos_pid_t pid;
  uint8_t numfx;
  uint8_t fnidx;
  uint16_t page;
  func_addr_t* fnptrarr;
} sfi_jmptblrec_t;

uint8_t sfi_domain_pid_map[SFI_MOD_DOMAINS];
sfi_jmptblrec_t jmptblrec;
uint16_t domain_stack_bound;
void* safe_stack_ptr;

#ifdef SFI_DOMS_8
uint8_t curr_dom_id;
#endif

static inline int8_t get_free_domain_id(sos_pid_t pid)
{
  int8_t idx, ret;
  ret = -1;
  for (idx = 0; idx < SFI_MOD_DOMAINS; idx++){
    if (pid == sfi_domain_pid_map[idx])
      return idx;
    if (NULL_PID == sfi_domain_pid_map[idx])
      ret = idx;
  }
  return ret;
}

static inline int8_t get_domain_id_for_pid(sos_pid_t pid)
{
  int8_t idx;
  for (idx = 0; idx < SFI_MOD_DOMAINS; idx++){
    if (pid == sfi_domain_pid_map[idx])
      return idx;
  }
  return -1;
}


int8_t sfi_get_domain_id(sos_pid_t pid)
{
  if (pid < APP_MOD_MIN_PID)
    return KER_DOM_ID;
#ifdef SFI_DOMS_2
  return MOD_DOM_ID;
#endif
#ifdef SFI_DOMS_8
  return get_domain_id_for_pid(pid);
#endif
}

func_addr_t sfi_modtable_get_real_addr(func_addr_t addr)
{
  uint32_t byteaddr;
  uint16_t readword;
  func_addr_t realaddr;
  // Ram - For some reason a module that has not been loaded is propagated in avrora
  // This is a quickfix to get the simulation underway ... need to findout why that happens
  if (NULL != jmptblrec.fnptrarr){
    uint8_t fnidx;
    fnidx = (addr - (jmptblrec.page * WORDS_PER_PAGE))/(SIZE_OF_JMP_INSTR);
    return jmptblrec.fnptrarr[fnidx];
  }

  // The actual address is located one word after the input address
  // Because the entries in the table are jmp <fnaddr>
  byteaddr = (uint32_t)((uint32_t)addr << 1);
  byteaddr += 2;
  readword = pgm_read_word_far(byteaddr);
  realaddr = (func_addr_t)readword;
  return realaddr;
}

int8_t sfi_modtable_init()
{
  uint8_t idx;
  HAS_CRITICAL_SECTION;

#ifdef SFI_DOMS_8
  curr_dom_id = KER_DOM_ID;
#endif
  safe_stack_ptr = &__heap_start;
  ENTER_CRITICAL_SECTION();
  {
    domain_stack_bound = (uint16_t)((uint16_t)SPH << 8) | ((uint16_t)SPL);
  }
  LEAVE_CRITICAL_SECTION();

  for (idx = 0; idx < SFI_MOD_DOMAINS; idx++){
    sfi_domain_pid_map[idx] = NULL_PID;
  }
  jmptblrec.pid = NULL_PID;
  jmptblrec.numfx = 0;
  jmptblrec.fnidx = 0;
  jmptblrec.page = 0;
  jmptblrec.fnptrarr = NULL;
  return SOS_OK;
}


#ifdef MINIELF_LOADER
void sfi_modtable_register(codemem_t h)
#else
void sfi_modtable_register(mod_header_t* mhdr)
#endif
{
  int8_t domainid;
  func_addr_t module_handler;
#ifdef MINIELF_LOADER
  uint16_t mod_hdr_offset, mod_hdr_size, mod_handler_word_addr;
  uint32_t mod_start_addr;
  mod_header_ptr p;
  mod_header_t* mhdr;
  mod_start_addr = ker_codemem_get_start_address(h);
  p = ker_codemem_get_header_address(h);
  mod_handler_word_addr = sos_read_header_ptr(p, offsetof(mod_header_t, module_handler));
  mod_hdr_size = (uint16_t)((mod_handler_word_addr - p) << 1);
  mod_hdr_offset = (uint16_t)((uint32_t)((uint32_t)p << 1) - mod_start_addr);
  mhdr = (mod_header_t*)ker_malloc(mod_hdr_size, KER_DFT_LOADER_PID);
  ker_codemem_read(h, KER_DFT_LOADER_PID, mhdr, mod_hdr_size, mod_hdr_offset);
#endif

  domainid = get_free_domain_id(mhdr->mod_id);
  // Ram - TODO error handling 
  if (domainid < 0)
    sfi_exception(SFI_DOMAINID_EXCEPTION);
  // Update the domain ID to module ID map
  sfi_domain_pid_map[domainid] = mhdr->mod_id;
  jmptblrec.pid = mhdr->mod_id;
  jmptblrec.numfx = mhdr->num_sub_func + mhdr->num_prov_func + 1; // Extra function is module handler
  jmptblrec.fnidx = 0;
  jmptblrec.page = CONV_DOMAINID_TO_PAGENUM(domainid);
  jmptblrec.fnptrarr = (func_addr_t*)ker_malloc(sizeof(func_addr_t) * jmptblrec.numfx,
						KER_DFT_LOADER_PID);
  
  module_handler = (func_addr_t)mhdr->module_handler;
  mhdr->module_handler = (void*)sfi_modtable_add(module_handler); 
#ifdef MINIELF_LOADER
  uint8_t n;
  for (n = 0; n < (jmptblrec.numfx - 1); n++) {
    func_addr_t func_loc = offsetof(mod_header_t, funct) + 
      (n * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr);
    dummy_func *f = (dummy_func*)((uint8_t*)mhdr + func_loc);
    *f = (dummy_func)sfi_modtable_add((func_addr_t)*f);
  }
  ker_codemem_write(h, KER_DFT_LOADER_PID, mhdr, mod_hdr_size, mod_hdr_offset);
  ker_codemem_flush(h, KER_DFT_LOADER_PID);
#endif
  return;			       
}




void sfi_modtable_deregister(sos_pid_t pid)
{
  int8_t domainid;
  uint8_t wordidx;
  //  uint16_t modpage;
  uint32_t modpageaddr;
  uint16_t sfimodtable[WORDS_PER_PAGE];
  
  domainid = get_domain_id_for_pid(pid);
  if (domainid < 0)
    return;
  sfi_domain_pid_map[domainid] = NULL_PID;

  
  // Fill up the rest of the table with jmp sfi_exception
  for (wordidx = 0; wordidx < WORDS_PER_PAGE; wordidx += 2){
    sfimodtable[wordidx] = (OP_JMP & OP_TYPE10_MASK);
    sfimodtable[wordidx+1] = (uint16_t)sfi_jmptbl_exception;
  }
  // Get module page number
  // Ram - Replace with new FLASH API
  //  modpage = CONV_DOMAINID_TO_PAGENUM(domainid);
  //  FlashWritePage(modpage, (uint8_t*)sfimodtable, WORDS_PER_PAGE * 2);
  modpageaddr = CONV_DOMAINID_TO_ADDRESS(domainid);
  flash_write(modpageaddr, (uint8_t*)sfimodtable, WORDS_PER_PAGE * 2);
  return;
}


func_addr_t sfi_modtable_add(func_addr_t addr)
{
  func_addr_t modtable_addr;
  if (jmptblrec.fnidx < jmptblrec.numfx)
    jmptblrec.fnptrarr[jmptblrec.fnidx] = addr;
  else
    sfi_exception(SFI_DOMAINID_EXCEPTION);
  modtable_addr = (func_addr_t)(SFI_MOD_TABLE_ENTRY_LOC(jmptblrec.page, jmptblrec.fnidx));
  jmptblrec.fnidx++;
  return modtable_addr;
}

void sfi_modtable_flash(mod_header_ptr h)
{
  sos_pid_t pid;
  uint16_t sfimodtable[WORDS_PER_PAGE];
  uint8_t fnidx, wordidx;
  uint32_t flash_write_addr;
  //  uint16_t* sfimodtable;
  fnidx = 0;
  wordidx = 0;
  //  sfimodtable = (uint16_t*)ker_malloc(sizeof(uint16_t) * WORDS_PER_PAGE,
  //				      KER_DFT_LOADER_PID);
				      
  pid = sos_read_header_byte(h, offsetof(mod_header_t, mod_id));
 
 // Ram - Safety check - If we are flashing the correct module table
  if (jmptblrec.pid != pid)
    sfi_exception(SFI_DOMAINID_EXCEPTION);
  // Ram - Safety check - If we jmptblrec has all new function addresses
  if (jmptblrec.fnidx != jmptblrec.numfx)
    sfi_exception(SFI_DOMAINID_EXCEPTION);

  // Fill up the jump table with module specific addresses
  for (fnidx = 0; fnidx < jmptblrec.numfx; fnidx++){
    sfimodtable[wordidx] = (OP_JMP & OP_TYPE10_MASK);
    wordidx++;
    sfimodtable[wordidx] = jmptblrec.fnptrarr[fnidx];
    wordidx++;
  }

  // Fill up the rest of the table with jmp sfi_exception
  for (; wordidx < WORDS_PER_PAGE; wordidx += 2){
    sfimodtable[wordidx] = (OP_JMP & OP_TYPE10_MASK);
    sfimodtable[wordidx+1] = (uint16_t)sfi_jmptbl_exception;
  }

  //  FlashWritePage(jmptblrec.page, (uint8_t*)sfimodtable, WORDS_PER_PAGE * 2);
  flash_write_addr = (uint32_t)jmptblrec.page * 256;
  flash_write(flash_write_addr, (uint8_t*)sfimodtable, WORDS_PER_PAGE * 2);

  jmptblrec.pid = NULL_PID;
  jmptblrec.numfx = 0;
  jmptblrec.fnidx = 0;
  jmptblrec.page = 0;
  ker_free(jmptblrec.fnptrarr);
  //  ker_free(sfimodtable);
  jmptblrec.fnptrarr = NULL;
  return;
}

//----------------------------------------------------------
// SFI EXCEPTION HANDLER
//----------------------------------------------------------
void sfi_jmptbl_exception()
{
  uint16_t val;
  uint8_t ledcnt;
  ker_led(LED_RED_ON);
  ker_led(LED_GREEN_OFF);
  ker_led(LED_YELLOW_OFF);
  val = 0xffff;
  ledcnt = 0;
  while (1){
#ifndef DISABLE_WDT
    watchdog_reset();
#endif
    if (val == 0){
      switch (ledcnt){
      case 0: ker_led(LED_RED_TOGGLE); ker_led(LED_GREEN_TOGGLE); break;
      case 1: ker_led(LED_GREEN_TOGGLE); ker_led(LED_YELLOW_TOGGLE); break;
      case 2: ker_led(LED_YELLOW_TOGGLE); ker_led(LED_RED_TOGGLE); break;
      default: break;
      }
      ledcnt++;
      if (ledcnt == 3) 
	ledcnt = 0;
    }
    val--;
  }
  return;
}

static void sfi_err_code_led_display(uint8_t errcode)
{
  uint8_t _led_display;				
  _led_display = errcode;				
  if (_led_display & 0x01)			
    led_yellow_on();				
  else					
    led_yellow_off();				
  _led_display >>= 1;				
  if (_led_display & 0x01)			
    led_green_on();				
  else					
    led_green_off();				
  _led_display >>= 1;				
  if (_led_display & 0x01)			
    led_red_on();				
  else					
    led_red_off();
  return;
}

void sfi_exception(uint8_t errcode)
{
  uint8_t clrdisp;
  uint16_t val;
  val = 0xffff;

  sfi_err_code_led_display(errcode);
  clrdisp = 0;

  while (1){
#ifndef DISABLE_WDT
    watchdog_reset();
#endif
    if (val == 0){
      if (clrdisp){
	sfi_err_code_led_display(errcode);
	clrdisp = 0;
      }
      else {
	sfi_err_code_led_display(0);
	clrdisp = 1;
      }
    }
    val--;
  }
  return;
}
