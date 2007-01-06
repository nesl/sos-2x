/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @brief    dbg module for protosb
 * @author   Naim Busek <ndbusek@gmail.com>
 * @version  0.1
 *
 */

#include <sos.h>
#include "dbg.h"

int8_t ker_dbg(uint8_t action){
	switch (action){
		case DBG_BIT0_SET:       dbg_bit0_set(); break;
		case DBG_BIT1_SET:       dbg_bit1_set(); break;
		case DBG_BIT2_SET:       dbg_bit2_set(); break;
		case DBG_BIT3_SET:       dbg_bit2_set(); break;
		case DBG_BIT0_CLR:       dbg_bit0_clr(); break;
		case DBG_BIT1_CLR:       dbg_bit1_clr(); break;
		case DBG_BIT2_CLR:       dbg_bit2_clr(); break;
		case DBG_BIT3_CLR:       dbg_bit2_clr(); break;
		case DBG_BIT0_TOGGLE:    dbg_bit0_toggle(); break;
		case DBG_BIT1_TOGGLE:    dbg_bit1_toggle(); break;
		case DBG_BIT2_TOGGLE:    dbg_bit2_toggle(); break;
		case DBG_BIT3_TOGGLE:    dbg_bit2_toggle(); break;
	}
	return SOS_OK;
}
