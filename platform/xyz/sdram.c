#include <hardware.h>
#include "sdram.h"

/****************************************************************************/
/*  Setup of external SRAM/ROM                                              */
/*  Function : configure_SRAM	                                            */
/*      Parameters                                                          */
/*          Input   :   Nothing                                             */
/*          Output  :   Nothing                                             */
/****************************************************************************/
void sdram_init(void)
{
    put_wvalue(BWC, 0x28);	// set bus width to 16 bits
    //put_wvalue(ROMAC, 0x7);     // setup ROM access timing
    put_wvalue(RAMAC, 0x7);		// setup SRAM access timing
    return;
}

