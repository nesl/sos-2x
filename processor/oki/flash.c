/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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

#include <sos_inttypes.h>
#include <sos_types.h>
#include "hardware.h"
#include "flash.h"

#define	OK		(0)
#define ERROR	(-1)
#define NG      ERROR

#define NORMAL_DAT  (0)
#define END_DAT     (1)
#define UART_ERR    (2)

#define YES            (1)
#define NO             (0)
#define PROTECT_CANCEL (1)
#define PROTECT_SET    (2)
#define UNPROTECTED    (0)
#define PROTECTED      (1)

/* ASCII CODE of Xon/Xoff */
#define XON     17
#define XOFF    19

enum{
    CHIP_ID_READ=1,
    UNPROTECT,
    PROTECT,
    CHIP_ERASE,
    FLASH_WRITE
};

/****** DATA *******/
#define DAT_00  0x0000
#define DAT_01  0x0001
#define DAT_10  0x0010
#define DAT_30  0x0030
#define DAT_50  0x0050
#define DAT_55  0x0055
#define DAT_80  0x0080
#define DAT_90  0x0090
#define DAT_A0  0x00A0
#define DAT_AA  0x00AA
#define DAT_D0  0x00D0
#define DAT_E0  0x00E0
#define DAT_F0  0x00F0

/****** ADDRESS ******/
#define ADDR_2AA    (0x000002AA << 1)
#define ADDR_555    (0x00000555 << 1)
#define ADDR_XXX    (0x00000000)
#define BIT_PASS_W   0x00000080		// DQ7 for DATA_N polling (abs)

#define MANUFACT_ID {0x0062     , 0x00F0}
#define DEVICE_ID   {0x0002     /* 4Mbit */, 0x0046     /* 2Mbit */}
#define FLASH_SIZE  {(512*1024) /* 4Mbit */, (256*1024) /* 2Mbit */}
#define FUNCSIZE	1000	// must be large enough for largest flash function to run in ram
#define OKI_FLASH_SECTOR_SIZE	2048

/********************************************************************/
/*              Flash Memory Data Poling Proc                       */
/********************************************************************/
//static int subPoling(uint16_t *addr, uint16_t data)
//static int subPoling(uint16_t *addr, uint16_t data);
#define subPoling(add,dat,stat)	{ uint16_t pol;\
								  	pol = get_hvalue(add);\
    							 	while((pol & BIT_PASS_W) != (dat & BIT_PASS_W)) {pol = get_hvalue(add);}\
    								pol = get_hvalue(add);\
    								if((pol & BIT_PASS_W) != (dat & BIT_PASS_W)){*stat = -1;}\
    								else {*stat = 0;}\
									}

#define ResetFlash()		put_hvalue((FLASH_START + ADDR_XXX), DAT_F0)

#define VALUE_OF_TIMECMP    10 * (CCLK / 32000)    /* 10ms * (CCLK) / (32 * 1000) */

#define SetFlashTimer()  	{out_hw(TIMECNTL4, 0);\
    						out_hw(TIMESTAT4, TIMESTAT_STATUS);\
    						out_hw(TIMECNTL4,TIMECNTL_CLK32 | TIMECNTL_INT);\
    						out_hw(TIMEBASE4, 0x0000);\
    						out_hw(TIMECMP4, VALUE_OF_TIMECMP);\
    						out_hw(TIMECNTL4, TIMECNTL_START);}

/* functions */
static int ChipIDCheck(void);
//static int ProtectCheck(void);
//static int Unprotect(void);
//static int Protect(void);
//static void SetupTimer(void);
int fmChipInit(void);
int	fmManufactureCodeRead(void);
int	fmDeviceCodeRead(void);
int fmManufactureCodeCheck(int m_code);
int fmDeviceCodeCheck(int d_code);
int fmBlockProtectCheck(void);
int fmChipProtectCheck(void);
int fmBlockProtect(void);
int fmChipProtect(void);
int fmUnprotect(void);
int fmProtect(void);
int fmSectorErase(unsigned long sect_addr);
int fmBlockErase(unsigned long block_addr);
int fmChipErase(void);
int fmDataWrite(unsigned char *src_addr, unsigned long dst_addr, int write_size);
int fmSetChipID(int m_code, int d_code);

static int device_id[] = DEVICE_ID;
static int manufact_id[] = MANUFACT_ID;
static int flash_size[] = FLASH_SIZE;
static int MANUFACTURE_CODE = -1;
static int DEVICE_CODE = -1;
static unsigned int FLASH_START = 0xc8000000;    /* Flash Start Address */
unsigned long FuncBuf[FUNCSIZE];				// Can we use one array for all flash routines?
//static uint8_t flash_buf[OKI_FLASH_SECTOR_SIZE];

void flash_init(void)
{
    put_wvalue(ROMAC, 0x13);     // setup ROM access timing for OE/WE is 5 cycles and enable ROMBRST
    //put_wvalue(ROMAC, 0x3);     // setup ROM access timing
}

void FlashWritePage(uint16_t page, uint8_t *data, uint16_t len)
{
	//HAS_CRITICAL_SECTION;
	uint32_t dst_addr;
	uint16_t write_size;

	//ENTER_CRITICAL_SECTION();
	if(len > OKI_FLASH_SECTOR_SIZE)
		len = OKI_FLASH_SECTOR_SIZE;
	/* Check if page is in bounds! */
	if (fmChipInit() != OK) {
		return;// 0;	//error val
	}
    if(ChipIDCheck() != OK) {    /* Chip-ID Check */
        return;// CHIP_ID_READ;
    }
    //if(ProtectCheck() != OK) {    /* Protect Check */
    //    if(Unprotect() != OK) {    /* Protect Cancel */
    //        return;// UNPROTECT;
    //    }
    //}
    dst_addr = (uint32_t)page * OKI_FLASH_SECTOR_SIZE;
    if(fmSectorErase(dst_addr) == OK) {
		return;// CHIP_ERASE;
	}
    write_size = fmDataWrite(data, dst_addr, len);
    if(write_size != len && write_size != len+1) {
		return;// FLASH_WRITE;
	}
/*
    if(Protect() != OK) {    // Protect Set
        return;// PROTECT;
    }
*/
	//LEAVE_CRITICAL_SECTION();
}

/**
 * @brief load flash buffer from flash
 * @param page the page number in the flash
 * @param section the seciton number in the flash
 *
 */
void FlashReadPage(uint16_t page, uint16_t start, uint8_t *data, uint16_t size)
{
	HAS_CRITICAL_SECTION;
	uint32_t dst_addr;
	uint16_t j;
	dst_addr = 	page * OKI_FLASH_SECTOR_SIZE + fmGetFlashStart();
	ENTER_CRITICAL_SECTION();
	for (j = 0; j < size; j++) {
		*data = get_value(dst_addr + start + j);
		++data;
	}
	LEAVE_CRITICAL_SECTION();
}

/**
 * @brief check the flash content against buffer
 */
int8_t FlashCheckPage(uint16_t page, uint8_t *data, uint16_t size)
{
	uint32_t dst_addr;
	uint16_t i;
	uint16_t *wp = (uint16_t*)data;
	dst_addr = 	page * OKI_FLASH_SECTOR_SIZE + fmGetFlashStart();
	for(i = 0; i < size; i += 2) {
		uint16_t buf = get_hvalue(dst_addr + i);
		if(buf != wp[i/2]) {
			return -EAGAIN;
		}
	}
	return 0;
}

/*
 *	CopyFuncToRam
 *
 */
int CopyFuncToRam(unsigned long *begin,unsigned long *end,unsigned long *copy,int size)
{
	volatile unsigned long	*bp;
	int	ret;

	DISABLE_CACHE();
	/* Verify space availability: */
	if (((int)end - (int)begin) >= size) {
		return(-1);
	}
	ret = 0;
	/* Copy function() to RAM, then verify: */
	bp = begin;
	while(bp <= end) {
		*copy = *bp;
		if (*copy++ != *bp++) {
			ret = -1;
			break;
		}
	}
	ENABLE_CACHE();			// should we check if it was enabled before????
	return(ret);
}

/****************************************************************************/
/*  Protect Check Proc                                                      */
/*  Function : ProtectCheck                                                 */
/*      Parameters                                                          */
/*          Input   : Nothing                                               */
/*          Output  : OK/ERROR (=Unprotect/Protect)                         */
/****************************************************************************/
//static int ProtectCheck(void)
//{
//    int block_protect, chip_protect;
//
//    block_protect = fmBlockProtectCheck();       /* Get Block Protect Check */
//    chip_protect  = fmChipProtectCheck();        /* Get Chip Protect Check */
//
//    if(chip_protect == UNPROTECTED && block_protect == UNPROTECTED) {
//        return OK;
//    } else {
//        return ERROR;
//    }
//}

/****************************************************************************/
/*  Unprotect Proc                                                          */
/*  Function : Unprotect                                                    */
/*      Parameters                                                          */
/*          Input   : Nothing                                               */
/*          Output  : Protect cancel OK/ERROR                               */
/****************************************************************************/
//static int Unprotect(void)
//{
//	led_green_on();
//    fmUnprotect();
//	led_green_off();
//            				/* Block & Chip Protect Chech OK */
//    if((fmBlockProtectCheck() != PROTECTED) && (fmChipProtectCheck() != PROTECTED)) {
//        return OK;
//    }
//    /* Block or Chip Protect Chech NG */
//    return ERROR;
//}

/****************************************************************************/
/*  Protect Proc                                                            */
/*  Function : Protect                                                      */
/*      Parameters                                                          */
/*          Input   : Nothing                                               */
/*          Output  : Protect set OK/ERROR                                  */
/****************************************************************************/
//static int Protect(void)
//{
//    fmBlockProtect();        /* Block Protect */
//    fmChipProtect();         /* Chip Protect */
//            				/* Block & Chip Unprotect Chech OK */
//    if((fmBlockProtectCheck() != UNPROTECTED) && (fmChipProtectCheck() != UNPROTECTED)) {
//        return OK;
//    }
//    /* Block or Chip Unprotect Check NG */
//    return ERROR;
//}

/********************************************************************/
/*                  Chip ID Check Proc                              */
/********************************************************************/
static int ChipIDCheck(void)
{
    int chk_flag = OK;
	uint32_t ManufactureCode = 0;
	uint32_t DeviceCode = 0;

    /* Flash Memory ID Check */
    ManufactureCode = fmManufactureCodeRead();      /* Get ManufactureCode */
    DeviceCode = fmDeviceCodeRead();         /* Get DeviceCode */
    if(fmManufactureCodeCheck(ManufactureCode) != OK){    /* Manufacture Code Error */
        chk_flag = ERROR;
    }
    if(fmDeviceCodeCheck(DeviceCode) != OK){    /* Device Code Error */
        chk_flag = ERROR;
    }
    if(chk_flag != OK) {
        return ERROR;
    }
    fmSetChipID(ManufactureCode, DeviceCode);    /* ManufactureCode  */
    //FlashSize = fmGetFlashSize();
    return OK;
}

/********************************************************************/
/*                  Manufacture Code Read Proc                      */
/********************************************************************/
int StartfmManufactureCodeRead(void)
{
    int m_code;

    put_hvalue((FLASH_START + ADDR_555), DAT_AA);    /* ID Read Command */
    put_hvalue((FLASH_START + ADDR_2AA), DAT_55);
    put_hvalue((FLASH_START + ADDR_555), DAT_90);
    m_code = get_hvalue(FLASH_START);      /* Get ManufactureCode */
    /* Reset ID Read Mode */
    put_hvalue((FLASH_START + ADDR_XXX), DAT_F0);
	ResetFlash();
	return m_code;
}
void EndfmManufactureCodeRead(void) {}

int fmManufactureCodeRead(void)
{
	int (*func)(void), m_code;

	if (CopyFuncToRam((unsigned long *)StartfmManufactureCodeRead,(unsigned long *)EndfmManufactureCodeRead,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	//return func();
	DISABLE_GLOBAL_INTERRUPTS();
	m_code = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return m_code;
}

/********************************************************************/
/*                  Device Code Read Proc                           */
/********************************************************************/
int StartfmDeviceCodeRead(void)
{
    int d_code;

    put_hvalue((FLASH_START + ADDR_555), DAT_AA);    /* ID Read Command */
    put_hvalue((FLASH_START + ADDR_2AA), DAT_55);
    put_hvalue((FLASH_START + ADDR_555), DAT_90);
    d_code = get_hvalue(FLASH_START+2);         /* Get DeviceCode */
    /* Reset ID Read Mode */
    put_hvalue(FLASH_START, (0x00ff & DAT_F0));
	ResetFlash();
    return d_code;
}
void EndfmDeviceCodeRead(void) {}

int fmDeviceCodeRead(void)
{
	int (*func)(void), d_code;

	if (CopyFuncToRam((unsigned long *)StartfmDeviceCodeRead,(unsigned long *)EndfmDeviceCodeRead,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	d_code = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return d_code;
}

/********************************************************************/
/*                  Manufacture Code Check Proc                     */
/********************************************************************/
int fmManufactureCodeCheck(int m_code)
{
    int i;

    for(i=0; i<(sizeof(manufact_id)/sizeof(manufact_id[i])); i++){
        if(m_code == manufact_id[i]){          /* Manufacture Code Error */
            return OK;
        }
    }
    return ERROR;
}

/********************************************************************/
/*                  Device Code Check Proc                          */
/********************************************************************/
int fmDeviceCodeCheck(int d_code)
{
    int i;

    for(i=0; i<(sizeof(device_id)/sizeof(device_id[0])); i++){
        if(d_code == device_id[i]){
            return OK;
        }
    }
    return ERROR;
}

/********************************************************************/
/*                  Block Protect Check Proc                        */
/********************************************************************/
int StartfmBlockProtectCheck(void)
{
    int block_protect;

    /* Flash Memory Unprotect */

    out_hw((FLASH_START + ADDR_555), DAT_AA);    /* Protect Check Command */
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_90);
    block_protect = in_hw(FLASH_START+4);      /* Get Block Protect State */
    /* Reset Protect Check Mode */
    out_hw((FLASH_START + ADDR_XXX), DAT_F0);
	ResetFlash();
    if(block_protect == 0){
        return UNPROTECTED;
    }else{
        return PROTECTED;
    }
}
void EndfmBlockProtectCheck(void) {}

int fmBlockProtectCheck(void)
{
	int (*func)(void),res;
	if (CopyFuncToRam((unsigned long *)StartfmBlockProtectCheck,(unsigned long *)EndfmBlockProtectCheck,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*                  Chip Protect Check Proc                         */
/********************************************************************/
int StartfmChipProtectCheck(void)
{
    int chip_protect;

    /* Flash Memory Unprotect */
    out_hw((FLASH_START + ADDR_555), DAT_AA);    /* Protect Check Command */
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_90);
    chip_protect = in_hw(FLASH_START+6);         /* Get Chip Protect State */
    /* Reset Protect Check Mode */
    out_hw((FLASH_START + ADDR_XXX), DAT_F0);
	ResetFlash();
    if(chip_protect == 0){
        return UNPROTECTED;
    }else{
        return PROTECTED;
    }
}
void EndfmChipProtectCheck(void) {}

int fmChipProtectCheck(void)
{
	int (*func)(void),res;
	if (CopyFuncToRam((unsigned long *)StartfmChipProtectCheck,(unsigned long *)EndfmChipProtectCheck,FuncBuf,sizeof(FuncBuf)) < 0)
		return ERROR;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*                  Protect Proc                                    */
/********************************************************************/
int fmProtect(void)
{
    fmBlockProtect();        /* Block Protect */
    fmChipProtect();         /* Chip Protect */
    return OK;
}

/********************************************************************/
/*                  Unprotect Proc                                  */
/********************************************************************/
int StartfmUnprotect(void)
{
    unsigned int time0, time1, count;

    /* Block & Chip Unprotect */
    out_hw((FLASH_START + ADDR_555), DAT_AA);    /* Unprotect Command */
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_E0);
    out_hw((FLASH_START + ADDR_XXX), DAT_01);
    for(count=0; count<1000; count++) {  /* 100ms */
        SetFlashTimer();    				/***  Setup TIMER  ***/
        time0 = in_hw(TIMECNT4);    		/* timer0 counter register read */
        do {
            time1 =in_hw(TIMECNT4);    		/* timer0 counter register read */
        } while((time1 - time0) < MILI_SEC_10);   /* 10ms */
    }
	ResetFlash();
    return OK;
}
void EndfmUnprotect(void) {}

int fmUnprotect(void)
{
	int (*func)(void), res;

	if (CopyFuncToRam((unsigned long *)StartfmUnprotect,(unsigned long *)EndfmUnprotect,FuncBuf,sizeof(FuncBuf)) < 0)
		return ERROR;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*                  Flash Memory Erase Proc                         */
/********************************************************************/
int StartfmChipErase(void)
{
	int stat;

    /* Flash Memory Bank  Erase */
    out_hw((FLASH_START + ADDR_555), DAT_AA);    /* Flash Erase Command */
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_80);
    out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_10);
    subPoling((uint16_t*)FLASH_START,(uint32_t)BIT_PASS_W,&stat);
	ResetFlash();
    if (stat != OK){	/* Check Erase End */
        return ERROR;
    }
    return OK;
}
void EndfmChipErase(void) {}

int fmChipErase(void)
{
	int (*func)(void),res;
	if (CopyFuncToRam((unsigned long *)StartfmChipErase,(unsigned long *)EndfmChipErase,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*                  Flash Memory Write Proc                         */
/********************************************************************/
//int StartfmDataWrite(unsigned char *src_addr, unsigned long dst_addr, int write_size)
int StartfmDataWrite(uint8_t *src_addr, uint32_t dst_addr, uint16_t write_size)
{
    uint16_t *write_addr = (uint16_t *)(FLASH_START + dst_addr);
    //unsigned short *write_addr = (unsigned short *)(FLASH_START + dst_addr);
    //unsigned short *read_addr = (unsigned short *)src_addr;
    //unsigned short write_data;
    uint16_t write_data;
    uint16_t i;
    uint16_t size = 0;
    int stat;
    uint16_t a,b;

    //if((write_size&1) == 1){
    //    src_addr[write_size] = 0xFF;
    //    write_size++;
    //}
    for(i=0; i<write_size; i+=2, write_addr++){	//, read_addr++){
    	wdt_reset();
    	a = src_addr[i];
    	if (i < write_size)
    		b = src_addr[i+1];
    	else b = 0;
    	//write_data = (a << 8) | b;	//get_hvalue(read_addr);
    	write_data =  (b << 8) | a;		// problem with endian, flip here?????
        put_hvalue((FLASH_START + ADDR_555), DAT_AA);   /* Flash Write Command */
        put_hvalue((FLASH_START + ADDR_2AA), DAT_55);
        put_hvalue((FLASH_START + ADDR_555), DAT_A0);
        put_hvalue(write_addr, write_data);	/* Data Write */
        //if(subPoling(write_addr, write_data) < 0){
        subPoling(write_addr, write_data, &stat);
        if (stat < 0){				/* Check Write End */
            break;
        }
        size += 2;
    }
	//for(i=0; i<1000; i++);
	ResetFlash();
    return size;
}
void EndfmDataWrite(void) {}

int fmDataWrite(unsigned char *src_addr, unsigned long dst_addr, int write_size)
{
	int (*func)(unsigned char *,unsigned long,int), res;

	if (CopyFuncToRam((unsigned long *)StartfmDataWrite,(unsigned long *)EndfmDataWrite,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE ????
	func = (int(*)(unsigned char *,unsigned long,int))FuncBuf;
	//wdt_disable();
	DISABLE_GLOBAL_INTERRUPTS();
	res = func(src_addr, dst_addr, write_size);
	ENABLE_GLOBAL_INTERRUPTS();
	//wdt_enable();
	return res;
}

/********************************************************************/
/*              Flash Memory Initialize Proc                        */
/********************************************************************/
int fmChipInit(void)
{
	//put_hvalue((FLASH_START + ADDR_XXX), DAT_F0);
	ResetFlash();
    return OK;
}

/********************************************************************/
/*              Flash Memory Chip Protect Proc                      */
/********************************************************************/
int StartfmChipProtect(void)
{
    unsigned int time0, time1;

    out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_D0);
    out_hw((FLASH_START + ADDR_XXX), DAT_00);
    SetFlashTimer();    					/***  Setup TIMER  ***/
    time0 = in_hw(TIMECNT4);    			/* timer0 counter register read */
    do {
        time1 =in_hw(TIMECNT4);    			/* timer0 counter register read */
    } while(time1 - time0 < NANO_SEC_30);   /* 30us */
    out_hw(TIMECNTL4, TIMECNTL_INT);    	/* stop timer */
	ResetFlash();
    return OK;
}
void EndfmChipProtect(void) {}

int fmChipProtect(void)
{
	int (*func)(void),res;
	if (CopyFuncToRam((unsigned long *)StartfmChipProtect,(unsigned long *)EndfmChipProtect,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*              Flash Memory Block Protect Proc                     */
/********************************************************************/
int StartfmBlockProtect(void)
{
    unsigned int time0, time1;

	out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_E0);
    out_hw((FLASH_START + ADDR_XXX), DAT_00);
    SetFlashTimer();    						/***  Setup TIMER  ***/
    time0 = (unsigned int)in_hw(TIMECNT4);    /* timer0 counter register read */
    do {
       	time1 = (unsigned int)in_hw(TIMECNT4);    /* timer0 counter register read */
    } while((time1 - time0) < NANO_SEC_30);   /* 30us */
	ResetFlash();
    return OK;
}
void EndfmBlockProtect(void) {}

int fmBlockProtect(void)
{
	int (*func)(void),res;
	if (CopyFuncToRam((unsigned long *)StartfmBlockProtect,(unsigned long *)EndfmBlockProtect,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(void))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func();
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*              Flash Memory Sector Erase Proc                      */
/********************************************************************/
int StartfmSectorErase(unsigned long sect_addr)
{
//	int stat;
	int i;

    out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_80);
    out_hw((FLASH_START + ADDR_555), DAT_AA);
    //out_hw((FLASH_START + ADDR_2AA), DAT_50);		// original code
    out_hw((FLASH_START + ADDR_2AA), DAT_55);		// data sheet
    out_hw((FLASH_START + (sect_addr & 0x0003F800)), DAT_30);
    //subPoling((uint16_t*)FLASH_START, (uint32_t)BIT_PASS_W, &stat);
	for(i=0; i<100000; i++) {
		wdt_reset();
	}
	ResetFlash();

//    if(stat != OK){	/* Check Erase End */
//        return ERROR;
//    }

    return OK;
}
void EndfmSectorErase(void) {}

int fmSectorErase(unsigned long sect_addr)
{
	int (*func)(unsigned long), res;

	if (CopyFuncToRam((unsigned long *)StartfmSectorErase,(unsigned long *)EndfmSectorErase,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(unsigned long))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func(sect_addr);
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*              Flash Memory Block Erase Proc                       */
/********************************************************************/
int StartfmBlockErase(unsigned long block_addr)
{
	int stat;

    out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + ADDR_555), DAT_80);
    out_hw((FLASH_START + ADDR_555), DAT_AA);
    out_hw((FLASH_START + ADDR_2AA), DAT_55);
    out_hw((FLASH_START + (block_addr & 0x00030000)), DAT_50);
    subPoling((uint16_t*)FLASH_START, (uint32_t)BIT_PASS_W, &stat);
	ResetFlash();
    if (stat != OK){	/* Check Erase End */
        return ERROR;
    }
    return OK;
}
void EndfmBlockErase(void) {}

int fmBlockErase(unsigned long block_addr)
{
	int (*func)(unsigned long), res;
	if (CopyFuncToRam((unsigned long *)StartfmBlockErase,(unsigned long *)EndfmBlockErase,FuncBuf,sizeof(FuncBuf)) < 0)
		return 0;		// ERROR CODE???
	func = (int(*)(unsigned long))FuncBuf;
	DISABLE_GLOBAL_INTERRUPTS();
	res = func(block_addr);
	ENABLE_GLOBAL_INTERRUPTS();
	return res;
}

/********************************************************************/
/*              Set Chip ID Proc                                    */
/********************************************************************/
int fmSetChipID(int m_code, int d_code)
{
    MANUFACTURE_CODE = m_code;
    DEVICE_CODE = d_code;
    return OK;
}

/********************************************************************/
/*              Get Flash Memory Size Proc                          */
/********************************************************************/
int fmGetFlashSize(void)
{
    int i;
    int chk_flag = ERROR;

    for(i=0; i<(sizeof(manufact_id)/sizeof(manufact_id[i])); i++){
        if(MANUFACTURE_CODE == manufact_id[i]){
            chk_flag = OK;
        }
    }
    if(chk_flag == OK){
        for(i=0; i<(sizeof(device_id)/sizeof(device_id[0])); i++){
            if(DEVICE_CODE == device_id[i]){
                return flash_size[i];
            }
        }
    }
    return ERROR;
}

/********************************************************************/
/*              Get Flash Memory Start Address Proc                 */
/********************************************************************/
unsigned long fmGetFlashStart(void)
{
    return FLASH_START;
}
