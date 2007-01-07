/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @brief    Function Pointer Table
 * @author   Ram Kumar (ram@ee.ucla.edu)
 */

#include <sos_types.h>
#include <sos_sched.h>
#include <malloc.h>
#include <message.h>
#include <sos_module_types.h>
#include <sos_linker_conf.h>
#include <fntable_types.h>
#include <fntable.h>
#ifdef SOS_SFI
#include <sfi_jumptable.h>
#endif

/*
#ifndef SOS_DEBUG_FN_TABLE
#undef DEBUG
#define DEBUG(...)
#endif
*/
//----------------------------------------------------------------------------
// Typedefs
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------


#ifndef QUALNET_PLATFORM
//! Local Functions
static bool check_proto(uint8_t *proto1, uint8_t *proto2);
static func_cb_ptr fntable_get_prov_cb(func_cb_ptr funct, uint8_t fid,
		uint8_t start, uint8_t end);
static func_cb_ptr fntable_real_subscribe(mod_header_ptr sub_h,
		sos_pid_t pub_pid, uint8_t fid, uint8_t table_index);
static void fntable_link_provided_functions(func_cb_ptr funct,
		uint8_t start, uint8_t end, bool link);
#endif


/**
 * @brief Initializes the function pointer list to NULL
 */
int8_t fntable_init()
{
    return SOS_OK;
}

/**
 * @brief subscribe to function pointer
 * @param sos_pid_t sub_pid Module requesting function. (User) *
 * @param sos_pid_t pub_pid Module implementing the requested function. (Provider)
 * @param uint8_t fid function id the module is going to subscribe
 * @param uint8_t table_index the index to the function record, starting zero, this is used for gettting function prototypes
 * @return errno
 *
 */
int8_t ker_fntable_subscribe(sos_pid_t sub_pid, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index)
{
	sos_module_t *mod;
	mod_header_ptr sub_h;
	func_cb_ptr cb;
	func_cb_ptr *cb_in_ram;

	mod = ker_get_module(sub_pid);
	if(mod == NULL) {	
		ker_panic();
		return -EINVAL;
	}

	sub_h = mod->header;
	cb_in_ram = (func_cb_ptr*)(mod->handler_state);

	cb = fntable_real_subscribe(sub_h, pub_pid, fid, table_index);
	if(cb != 0) {
		cb_in_ram[table_index] = cb;
	} else {
		cb_in_ram[table_index] = sos_get_header_member(sub_h,
				offsetof(mod_header_t, funct[table_index]));
	}
	return SOS_OK;
}

static func_cb_ptr fntable_real_subscribe(mod_header_ptr sub_h, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index)
{
	mod_header_ptr pub_h;
	uint8_t proto_pub[4];
	uint8_t proto_sub[4];
	func_cb_ptr pub_cb = 0;
	uint8_t i = 0;
	uint8_t num_sub_func;
	uint8_t num_prov_func;
	sos_module_t *mod;

	mod = ker_get_module(pub_pid);
	if(mod == NULL) return 0;
	pub_h = mod->header;
	num_sub_func = sos_read_header_byte(pub_h,
			offsetof(mod_header_t, num_sub_func));
	num_prov_func = sos_read_header_byte(pub_h,
			offsetof(mod_header_t, num_prov_func));
	pub_cb = fntable_get_prov_cb(
			sos_get_header_member(pub_h, offsetof(mod_header_t, funct)),
			fid, num_sub_func, num_sub_func + num_prov_func);
	if(pub_cb == 0) {
		return 0;
	}
	for(i = 0; i < 4; i++) {
		proto_pub[i] = sos_read_header_byte(pub_cb,
				offsetof(func_cb_t, proto[i]));
		proto_sub[i] = sos_read_header_byte(sub_h,
				offsetof(mod_header_t, funct[table_index].proto[i]));
	}
	if(check_proto(proto_pub, proto_sub)) {
		return pub_cb;
	}
	return 0;
}

static func_cb_ptr fntable_get_prov_cb(func_cb_ptr funct, uint8_t fid, uint8_t start, uint8_t end)
{
	uint8_t i;

	for(i = start; i < end; i++) {
		if(sos_read_header_byte(funct,
					i * sizeof(func_cb_t) + offsetof(func_cb_t, fid))
			   	== fid) {
			return sos_get_header_member(funct,
					i * sizeof(func_cb_t));
		}
	}
	return 0;
}


/**
 * @brief link the module into correct address
 * NOTE: this rountine assumes the header is in RAM
 * fix offsetof(mod_header_t, module_handler)
 * fix offsetof(mod_header_t, funct) + (n * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr) for n = [0 ... number_of_funcs) 
 */
static inline func_addr_t mod_header_size(uint8_t num_funcs)
{
	return offsetof(mod_header_t, funct) + 
		(num_funcs * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr);
}

#ifndef MINIELF_LOADER
void fntable_fix_address(
		func_addr_t  base_addr, 
		uint8_t      num_funcs, 
		uint8_t     *buf, 
		uint16_t     nbytes, 
		func_addr_t  offset)
#ifndef PC_PLATFORM
{
	func_addr_t addr;
	uint8_t n;
	if( offset > mod_header_size(num_funcs) ) {
		return;
	}
	// handle special case for module_handler
	if( offset == 0 && nbytes >= offsetof(mod_header_t, funct) ) {
		// patch module handler
		mod_header_t *hdr = (mod_header_t *) buf;
		addr = (func_addr_t) hdr->module_handler;
		addr += base_addr;
		hdr->module_handler = (void*)addr;
#ifdef SOS_SFI
		// Module Handler is changed by this call
		sfi_modtable_register(hdr);
#endif
	}

	for( n = 0; n < num_funcs; n++ ) {
		func_addr_t func_loc = offsetof(mod_header_t, funct) + 
			(n * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr);
		if( func_loc >= offset && 
			((func_loc + sizeof(dummy_func) - 1) < (offset + nbytes)) ) {
			dummy_func *f = (dummy_func*)(buf + func_loc - offset);
			addr = (func_addr_t) *f;
			addr += base_addr;
#ifdef SOS_SFI
			*f = (dummy_func)sfi_modtable_add((func_addr_t)addr);
			//			*f = (dummy_func) addr;
#else
			*f = (dummy_func) addr;
#endif
		}
	}
}
#else
{
	DEBUG("simulate fix_address on PC platform base_addr = %d, num_funcs = %d, nbytes = %d, offset = %d\n", base_addr, num_funcs, nbytes, offset);
}
#endif//PC_PLATFORM
#endif//MINIELF_LOADER

#if 0
int8_t fntable_fix_address(mod_header_t *hdr, func_addr_t base_addr)
{
	func_addr_t addr;
	uint8_t i;
	//! fix module handler
	addr = (func_addr_t) hdr->module_handler;
	addr += base_addr;
	hdr->module_handler = (void*)addr;
	//DEBUG("linker_link: base_addr = %d, num_sub_func = %d num_prov_func\n", base_addr, hdr->num_sub_func, hdr->num_prov_func);
	for(i = 0; i < hdr->num_sub_func; i++) {
		//! allow user to use its own error handler
		addr = (func_addr_t) hdr->funct[i].ptr;
		addr += base_addr;
		hdr->funct[i].ptr = (void*)addr;

	}
	for(i = hdr->num_sub_func;
			i < (hdr->num_sub_func + hdr->num_prov_func); i++) {
		addr = (func_addr_t) hdr->funct[i].ptr;
		addr += base_addr;
		hdr->funct[i].ptr = (void*)addr;
	}

	// NOTE: we delay function linking until the module is registered
	// so that we can handle the case that module is not activated
	return SOS_OK;
}
#endif

/**
 * @brief unlink the module from absolute address into position independent code
 * NOTE: this rountine assumes the header is in RAM
 * unfix offsetof(mod_header_t, module_handler)
 * unfix offsetof(mod_header_t, funct) + (n * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr) for n = [0 ... number_of_funcs) 
 *
 * @param hdr pointer to module header in RAM
 * @param base_addr base address
 */
void fntable_unfix_address(
		func_addr_t  base_addr, 
		uint8_t      num_funcs, 
		uint8_t     *buf, 
		uint16_t     nbytes, 
		func_addr_t  offset)
#ifndef PC_PLATFORM
{
	func_addr_t addr;
	uint8_t n;
	if( offset > mod_header_size(num_funcs) ) {
		return;
	}

	if( offset == 0 && nbytes >= offsetof(mod_header_t, funct) ) {
		// patch module handler
		mod_header_t *hdr = (mod_header_t *) buf;
#ifdef SOS_SFI
		addr = (func_addr_t)sfi_modtable_get_real_addr((func_addr_t) hdr->module_handler);
#else
		addr = (func_addr_t) hdr->module_handler;
#endif
		addr -= base_addr;
		hdr->module_handler = (void*)addr;
	}


	for( n = 0; n < num_funcs; n++ ) {
		func_addr_t func_loc = offsetof(mod_header_t, funct) + 
			(n * sizeof(func_cb_t)) + offsetof(func_cb_t, ptr);
		if( func_loc >= offset && 
			((func_loc + sizeof(dummy_func) - 1) < (offset + nbytes)) ) {
			dummy_func *f = (dummy_func*)(buf + func_loc - offset);
#ifdef SOS_SFI
			addr = (func_addr_t)sfi_modtable_get_real_addr((func_addr_t) (*f));
#else
			addr = (func_addr_t) *f;
#endif
			addr -= base_addr;
			*f = (dummy_func) addr;
		}
	}
}
#else
{
	DEBUG("simulate unfix_address on PC platform base_addr = %d, num_funcs = %d, nbytes = %d, offset = %d\n", base_addr, num_funcs, nbytes, offset);
}
#endif
#if 0
int8_t fntable_unfix_address(mod_header_t *hdr, func_addr_t base_addr)
{
	func_addr_t addr;
	uint8_t i;

	//! unfix module handler
	addr = (func_addr_t) hdr->module_handler;
	addr -= base_addr;
	hdr->module_handler = (void*)addr;
	//DEBUG("linker_unlink: base_addr = %d, num_sub_func = %d num_prov_func\n", base_addr, hdr->num_sub_func, hdr->num_prov_func);
	for(i = 0; i < hdr->num_sub_func; i++) {
		//! allow user to use its own error handler
		addr = (func_addr_t) hdr->funct[i].ptr;
		addr -= base_addr;
		hdr->funct[i].ptr = (void*)addr;
	}
	for(i = hdr->num_sub_func;
			i < (hdr->num_sub_func + hdr->num_prov_func); i++) {
		addr = (func_addr_t) hdr->funct[i].ptr;
		addr -= base_addr;
		hdr->funct[i].ptr = (void*)addr;
	}
	return SOS_OK;
}
#endif

/**
 * @brief link the functions
 */
int8_t fntable_link(sos_module_t *m)
{
   uint8_t num_prov_func;
   uint8_t num_sub_func;
   
   fntable_link_subscribed_functions(m);
   num_prov_func = sos_read_header_byte(m->header,
                                        offsetof(mod_header_t, num_prov_func));
   num_sub_func = sos_read_header_byte(m->header,
                                       offsetof(mod_header_t, num_sub_func));
   if(num_prov_func > 0) {
      fntable_link_provided_functions(
         sos_get_header_member(m->header, offsetof(mod_header_t, funct)),
         num_sub_func,
         num_sub_func + num_prov_func, true);
   }
   return SOS_OK;
}


void fntable_link_subscribed_functions(sos_module_t *m)
{
   uint8_t num_sub_func;
   uint8_t num_prov_func;
   uint8_t i;
   func_cb_ptr *cb_in_ram = (func_cb_ptr*)(m->handler_state);
   
   num_sub_func = sos_read_header_byte(m->header,
                                       offsetof(mod_header_t, num_sub_func));
   num_prov_func = sos_read_header_byte(m->header,
                                        offsetof(mod_header_t, num_prov_func));
   
   for(i = 0; i < num_sub_func; i++) {
      uint8_t pub_pid = sos_read_header_byte(m->header,
                                             offsetof(mod_header_t, funct[i].pid));
      uint8_t pub_fid = sos_read_header_byte(m->header,
                                             offsetof(mod_header_t, funct[i].fid));
      func_cb_ptr pub_cb = 0;
      if(pub_pid == m->pid) {
         pub_cb = fntable_get_prov_cb(
            sos_get_header_member(m->header,
                                  offsetof(mod_header_t, funct)),
            pub_fid,
            num_sub_func, (num_sub_func + num_prov_func));
      } else if(pub_pid != RUNTIME_PID) {
         pub_cb = fntable_real_subscribe(m->header, pub_pid, pub_fid, i);
      }
      if(pub_cb != 0) {
			cb_in_ram[i] = pub_cb;
		} else {
			cb_in_ram[i] = sos_get_header_member(m->header,
					offsetof(mod_header_t, funct[i]));
		}
	}
}


/**
 * @brief given provided function table, search ALL OTHER modules to find the match
 *
 * PROFILE NEEDED
 * NOTE: it will be good idea to profile this...
 */
static void fntable_link_provided_functions(func_cb_ptr funct, uint8_t start, uint8_t end, bool link)
{
	sos_module_t **all_modules = sched_get_all_module();
	uint8_t bin_itr;
	uint8_t pub_pid = sos_read_header_byte(
			//funct[start].pid);
			funct, start * sizeof(func_cb_t) + offsetof(func_cb_t, pid));

	//! search both function users and dyanmic functions
	for(bin_itr = 0; bin_itr < SCHED_NUMBER_BINS; bin_itr++) {
		sos_module_t *h = all_modules[bin_itr];
		while(h != NULL) {
			uint8_t num_sub = sos_read_header_byte(
					h->header, offsetof(mod_header_t, num_sub_func));
			uint8_t i;
			for(i = 0; i < num_sub; i++) {
				uint8_t sub_pid =
					sos_read_header_byte(
							h->header, offsetof(mod_header_t, funct[i].pid));
				if(sub_pid == pub_pid) {
					uint8_t sub_fid =
						sos_read_header_byte(
								h->header, offsetof(mod_header_t, funct[i].fid));
					func_cb_ptr pub_cb =
						fntable_get_prov_cb(funct, sub_fid, start, end);
					if(pub_cb != 0) {
						//! found function to be linked
						uint8_t proto_pub[4];
						uint8_t proto_sub[4];
						uint8_t j;
						func_cb_ptr *cb_in_ram =
							(func_cb_ptr*)(h->handler_state);
						for(j = 0; j < 4; j++) {
							proto_sub[j] =
								sos_read_header_byte(
										h->header,
										offsetof(mod_header_t, funct[i].proto[j]));
							proto_pub[j] = sos_read_header_byte(
									pub_cb, offsetof(func_cb_t, proto[j]));
						}
						if(link) {
							if(check_proto(proto_pub, proto_sub)) {
								cb_in_ram[i] = pub_cb;
							}
						} else {
							//! remove link
							cb_in_ram[i] =
								sos_get_header_member( h->header,
										offsetof(mod_header_t, funct[i]));
						}
					}
				} else if((sub_pid == RUNTIME_PID) && (link == false)) {
					func_cb_ptr *cb_in_ram = (func_cb_ptr *)(h->handler_state);
					func_cb_ptr sub_cb_in_ram = cb_in_ram[i];
					uint8_t k;
					//! compare to see whether there is a match
					for(k = start; k < end; k++) {
						if((funct + (k * sizeof(func_cb_t))) == sub_cb_in_ram) {
							//! found dynamic function
							cb_in_ram[i] =
								sos_get_header_member(h->header,
										offsetof(mod_header_t, funct[i]));
							//! send message back to subscriber
							post_short(h->pid, FNTABLE_PID, MSG_DFUNC_REMOVED,
									i, 0, 0);
							break;
						}
					}
				}
			}
			h = h->next;
		}
	}
}

int8_t fntable_remove_all(sos_module_t *m)
{
	uint8_t num_sub_func;
	uint8_t num_prov_func;
	uint8_t i;
	func_cb_ptr *cb_in_ram = (func_cb_ptr *)(m->handler_state);

	num_prov_func = sos_read_header_byte(m->header,
			offsetof(mod_header_t, num_prov_func));
	//! check whether there is any provided functions
	if(num_prov_func == 0) return SOS_OK;

	num_sub_func = sos_read_header_byte(
			m->header, offsetof(mod_header_t, num_sub_func));

	//! unlink all provided functions
	fntable_link_provided_functions(
			sos_get_header_member(m->header, offsetof(mod_header_t, funct)),
			num_sub_func,
			num_sub_func + num_prov_func, false);

	//! search all used functions and send a message to the providers as well
	//! send message to the function provider based on function pointer table.
	//! we can ignore RUNTIME_PID as provider cannot have it.
	for(i = 0; i < num_sub_func; i++) {
		func_cb_ptr pub_cb = cb_in_ram[i];
		uint8_t pub_pid = sos_read_header_byte(
				pub_cb, offsetof(func_cb_t, pid));
		if(pub_pid == m->pid || pub_pid == RUNTIME_PID) {
			//! if function pointer is pointing to self,
			//! we won't send remove message
			//! similarly if function pointer is pointing to RUNTIME_PID,
			//! this means that it is pointing to self
			continue;
		}
		post_short(pub_pid, FNTABLE_PID, MSG_DFUNC_REMOVED,
				m->pid, 0, 0);

	}
	return SOS_OK;
}


// Check that two function prototypes match
// NOTE: both prototypes are in read-only memory now
static bool check_proto(uint8_t *proto1, uint8_t *proto2)
{

	if ((proto1[0] == proto2[0]) &&
		(proto1[1] == proto2[1]) &&
		(proto1[2] == proto2[2]) &&
		(proto1[3] == proto2[3])) {
	   return true;
	}

	return false;
}

void error_v(func_cb_ptr p) 
{
	DEBUG("error_v is called\n");
}

int8_t error_8(func_cb_ptr p) 
{
	DEBUG("error_8 is called\n");
	return -1;
}	

int16_t error_16(func_cb_ptr p)
{
	DEBUG("error_16 is called\n");
	return -1;
}

int32_t error_32(func_cb_ptr p)
{
	DEBUG("error_32 is called\n");
	return -1;
}	

void* error_ptr(func_cb_ptr p)
{
	DEBUG("error_ptr is called\n");
	return NULL;
}

dummy_func ker_get_func_ptr(func_cb_ptr p, sos_pid_t *prev)
{
	if( prev != NULL ) {
		*prev = 
			ker_set_current_pid(sos_read_header_byte(p, offsetof(func_cb_t, pid)));
	}
	
	return (dummy_func)sos_read_header_ptr( p, offsetof(func_cb_t, ptr));
}

/**
 * Call to get the function pointer in the control block
 * It also sets the current pid to the function destination
 *
 */
dummy_func ker_sys_enter_func( func_cb_ptr p )
{
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();

	*pid_sp =
		ker_set_current_pid(sos_read_header_byte(p, offsetof(func_cb_t, pid)));

	pid_sp++;
	LEAVE_CRITICAL_SECTION();

	return (dummy_func)sos_read_header_ptr( p, offsetof(func_cb_t, ptr));
}                                                               

/**                                                             
 * Pop current_pid from the stack when func has finished execution
 */                                                             
void ker_sys_leave_func( void )                                 
{                                                               
	HAS_CRITICAL_SECTION;                                       

	ENTER_CRITICAL_SECTION();                                   
	pid_sp--;                                                   
	ker_set_current_pid( *pid_sp );                             
	LEAVE_CRITICAL_SECTION();                                   
}                    

int8_t ker_sys_fntable_subscribe( sos_pid_t pub_pid, uint8_t fid, uint8_t table_index ) 
{
	sos_pid_t my_id = ker_get_current_pid();

	return ker_fntable_subscribe( my_id, pub_pid, fid, table_index );
}

//! Error Stub Function
/*
   int8_t error_stub(func_cb_ptr proto_in, ...)
   {
   uint8_t j, num_args;
   va_list ap;
   int i;
   unsigned int ui;
   long l;
   unsigned long ul;
   double d;
   void* ptr;
  char proto[4];
  uint8_t itr;

  //! XXX
  DEBUG("received error_stub call\n");
  return -ENOSYS;
  va_start(ap, proto_in);
  for(itr = 0; itr < 4; itr++) {
	proto[itr] = sos_read_header_byte(
			proto_in, offsetof(func_cb_t, proto[itr]));
	DEBUG("proto[%d] = %c\n", itr, proto[itr]);
  }
  num_args = proto[3] - 47; //! 47 corresponds to character '0' in the ASCII table

  for (j = 1; j < num_args && j < 3; j++){
	switch (proto[j+1]){
	case 'c':
	case 's':
	  {
		// The above two would be promoted to a signed int of 16 bits
		i = va_arg(ap, int);
		break;
	  }
	case 'C':
	case 'S':
	  {
		ui = va_arg(ap, unsigned int);
		break;
	  }
	case 'l':
	  {
		l = va_arg(ap, long);
		break;
	  }
	case 'L':
	  {
		ul = va_arg(ap, unsigned long);
		break;
	  }

	case 'f':
	case 'o':
	  {
		d = va_arg(ap, double);
		break;
	  }
	case 'd': //! char*
	case 'D': //! unsigned char*
	case 'g': //! float*
	case 'm': //! long*
	case 'M': //! unsigned long*
	case 'p': //! double*
	case 't': //! short*
	case 'T': //! unsigned short*
	case 'w': //! void*
	case 'z': //! struct*
	  {
		// The pointer has the same size
		ptr = va_arg(ap, void*);
		break;
	  }
	case 'e': //! char* (dynamic)
	case 'E': //! unsigned char* (dynamic)
	case 'h': //! float* (dynamic)
	case 'n': //! long* (dynamic)
	case 'N': //! unsigned long (dynamic)
	case 'q': //! double* (dynamic)
	case 'u': //! short* (dynamic)
	case 'U': //! unsigned short* (dynamic)
	case 'x': //! void* (dynamic)
	case 'a': //! struct* (dynamic)
	  {
		// The pointer has the same size
		ptr = va_arg(ap, void*);
		// Now what ? Do we free the memory always ?
		ker_free(ptr);
		break;
	  }
	default:
	  break;
	}
  }
  return -ENOSYS;
}
*/

