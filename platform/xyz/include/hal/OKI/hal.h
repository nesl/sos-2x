/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                        CHIPCON HARDWARE ABSTRACTION LIBRARY FOR THE CC2420             *
 *      ***   + +   ***                               Library Header File                              *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * Copyright Chipcon AS, 2004                                                                          *
 *******************************************************************************************************
 * The Chipcon Hardware Abstraction Library is a collection of functions, macros and constants, which  *
 * can be used to ease access to the hardware on the CC2420 and the target microcontroller.            *
 *                                                                                                     *
 * All HAL library macros and constants are defined here. The file contains the following sections:    *
 *     - AVR<->CC2420 SPI interface (collection of simple and advanced SPI macros optimized for speed) *
 *     - Interrupts (macros used to enable, disable and clear various AVR interrupts)                  *
 *     - ADC (simple macros used to initialize and get samples from the AVR ADC)                       *
 *     - Pulse width modulator (simple macros used to initalize and use the 8-bit timer 0 for PWM)     *
 *     - Timer 0 (initialization for a single timeout)                                                 *
 *     - Timer 1 (initialization for a series of timeouts at a specified interval + start/stop)        *
 *     - Serial port (UART1) (Initialization + simple operation via macros)                            *
 *     - Useful stuff (macros and function prototypes)                                                 *
 *     - EEPROM access (function prototypes)                                                           *
 *     - Simple CC2420 functions (function prototypes)                                                 *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + the ATMEGA128(L) MCU                                            *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef HAL_H
#define HAL_H






/*******************************************************************************************************
 *******************************************************************************************************
 **************************            AVR<->CC2420 SPI INTERFACE             **************************
 *******************************************************************************************************
 *******************************************************************************************************/

#define DUMMY 0x00	/* Dummy value used for read back */

/* Wait till Busy bit in SSIO status register is cleared */
#define 	SSIO_WAIT_BUSY		while ((get_value(SSIOST) & SSIOSTA_BUSY) == SSIOSTA_BUSY)
/* Wait till Transmitter empty bit is set */
#define    SSIO_WAIT_TREMP		while((get_value(SSIOINT) & SSIOCON_TREMP) != SSIOCON_TREMP)


//-------------------------------------------------------------------------------------------------------
// Initialization

// Enables SPI
#define SPI_INIT() \
    do { \
	set_hbit(GPCTL, GPCTL_SSIO0);   	/* Set 2ndary function as SSIO port */\
    /***********************************************************/\
    /*    Setup of SSIO                                        */\
    /*    CON_SLMSB    corresponds to MSBfirst                 */\
    /*    CON_SFTSLV   corresponds to master                   */\
    /*    CON_SFTCLK   corresponds to 1/16cclk                 */\
    /************************************************************/\
    put_value(SSIOCON ,SSIOCON_SLMSB | SSIOCON_MASTER | SSIOCON_32CCLK);\
    /***********************************************************/\
    /*    Set Loopback test mode                               */\
    /***********************************************************/\
    put_value(SSIOTSCON ,SSIOTSCON_LBTST);\
    /***********************************************************/\
    /* work_dt = get_value(SSIOINT);                           */\
    /* work_dt = get_value(SSIOST);                            */\
    /* work_dt = get_value(SSIOTSCON);                         */\
    /* work_dt = get_value(SSIOINTEN);                         */\
    /***********************************************************/\
    /***********************************************************/\
    /*    Clear Status                                         */\
    /***********************************************************/\
    put_value(SSIOST,0x00);\
    /***********************************************************/\
    /* work_dt = get_value(SSIOINT);                           */\
    /* work_dt = get_value(SSIOST);                            */\
    /* work_dt = get_value(SSIOTSCON);                         */\
    /* work_dt = get_value(SSIOINTEN);                         */\
    /***********************************************************/\
    /***********************************************************/\
    /*    Dummy Data to Send & Receive                         */\
    /***********************************************************/\
    put_value(SSIOBUF, DUMMY);      /* Send dummy data */\
    /***********************************************************/\
    /* work_dt = get_value(SSIOINT);                           */\
    /* work_dt = get_value(SSIOST);                            */\
    /***********************************************************/\
    /*    Reset Loopback test mode                             */\
    /***********************************************************/\
    put_value(SSIOTSCON ,SSIOTSCON_NOTST);\
    /***********************************************************/\
    /*    Clear interrupt bit TRCMP & RXCMP                    */\
    /***********************************************************/\
    put_value(SSIOINT ,SSIOCON_TXCMP | SSIOCON_RXCMP);\
    } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// FAST SPI: Low level functions
//      x = value (BYTE or WORD)
//      p = pointer to the byte array to operate on
//      c = the byte count
//
// SPI_ENABLE() and SPI_DISABLE() are located in the devboard header file (CS_N uses GPIO)

#define FASTSPI_WAIT() \
    do { \
    	SSIO_WAIT_BUSY; /* Wait while SSIO status register busy bit is set */\
    } while (0)

#define FASTSPI_TX(x) \
    do { \
    	SSIO_WAIT_TREMP; /*Wait until buffer empty */\
    	put_value(SSIOINT, SSIOCON_TREMP);  /* clean TREMP bit */\
	    put_value(SSIOBUF, x);      /* Set send data */\
        FASTSPI_WAIT(); \
    } while (0)

#define FASTSPI_RX(x) \
    do { \
        FASTSPI_TX(DUMMY);      /* DUMMY data */\
        FASTSPI_WAIT();\
    	x = get_value(SSIOBUF);     /* Read received data */\
    } while (0)

#define FASTSPI_RX_GARBAGE() \
    do { \
        FASTSPI_TX(DUMMY); /*DUMMY data*/\
        FASTSPI_WAIT(); \
    } while (0)

#define FASTSPI_TX_WORD_LE(x) \
    do { \
        FASTSPI_TX(x); \
        FASTSPI_TX((x) >> 8); \
    } while (0)

#define FASTSPI_TX_WORD(x) \
    do { \
        FASTSPI_TX(((UHWORD)(x)) >> 8); \
        FASTSPI_TX((UBYTE)(x)); \
    } while (0)

#define FASTSPI_TX_MANY(p,c) \
    do { \
        for (UINT8 spiCnt = 0; spiCnt < (c); spiCnt++) { \
            FASTSPI_TX(((UBYTE*)(p))[spiCnt]); \
        } \
    } while (0)

#define FASTSPI_RX_WORD_LE(x) \
    do { \
        UBYTE temp;\
		FASTSPI_RX(temp);\
		x = temp;     /* Read received high byte */\
		FASTSPI_RX(temp);\
        x |= temp << 8;     /* Read received low byte */\
    } while (0)

#define FASTSPI_RX_WORD(x) \
    do { \
        UBYTE temp;\
        FASTSPI_RX(temp);\
        x = temp << 8;     /* Read received high byte */\
        FASTSPI_RX(temp);\
        x |= temp;     /* Read received low byte */\
    } while (0)

#define FASTSPI_RX_REG_WORD(x) \
    do { \
		UBYTE temp;\
        FASTSPI_RX(temp);\
        x = temp << 8;     /* Read received high byte */\
        FASTSPI_RX(temp);\
        x |= temp;     /* Read received low byte */\
    } while (0)

#define FASTSPI_RX_MANY(p,c) \
    do { \
        for (UINT8 spiCnt = 0; spiCnt < (c); spiCnt++) { \
            FASTSPI_RX((p)[spiCnt]); \
        } \
    } while (0)

// Register address:
#define FASTSPI_TX_ADDR(a) \
    do { \
        FASTSPI_TX(a);      /* Set send data */\
    } while (0)

// Register address:
#define FASTSPI_RX_ADDR(a) \
    do { \
        FASTSPI_TX((a) | 0x40);      /* Set send data */\
    } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  FAST SPI: Register access
//      s = command strobe
//      a = register address
//      v = register value

#define FASTSPI_STROBE(s) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX_ADDR(s); \
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_SETREG(a,v) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX_ADDR(a); \
        FASTSPI_TX((BYTE) ((v) >> 8)); \
        FASTSPI_TX((BYTE) (v)); \
        FASTSPI_TX_ADDR(DUMMY);	/* NOP to ensure that write is successfull */\
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_GETREG(a,v) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_RX_ADDR(a); \
        FASTSPI_RX_REG_WORD(v); \
        SPI_DISABLE(); \
    } while (0)

// Updates the SPI status byte
#define FASTSPI_UPD_STATUS(s) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX_ADDR(CC2420_SNOP); \
        FASTSPI_RX(s);\
        SPI_DISABLE(); \
    } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  FAST SPI: FIFO access
//      p = pointer to the byte array to be read/written
//      c = the number of bytes to read/write
//      b = single data byte

#define FASTSPI_WRITE_FIFO(p,c) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX_ADDR(CC2420_TXFIFO); \
        for (UINT8 spiCnt = 0; spiCnt < (c); spiCnt++) { \
            FASTSPI_TX(((UBYTE*)(p))[spiCnt]); \
        } \
        FASTSPI_TX(DUMMY); /*NOP*/\
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_FIFO(p,c) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_RX_ADDR(CC2420_RXFIFO); \
        for (UINT8 spiCnt = 0; spiCnt < (c); spiCnt++) { \
            FASTSPI_RX(((UBYTE*)(p))[spiCnt]); \
        } \
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_FIFO_BYTE(b) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_RX_ADDR(CC2420_RXFIFO); \
        FASTSPI_RX(b); \
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_FIFO_NO_WAIT(p,c) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_RX_ADDR(CC2420_RXFIFO); \
        for (UINT8 spiCnt = 0; spiCnt < (c); spiCnt++) { \
            FASTSPI_RX(((UBYTE*)(p))[spiCnt]); \
        } \
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_FIFO_GARBAGE(c) \
    do { \
	UINT8 spiCnt = 0;      \
        SPI_ENABLE(); \
        FASTSPI_RX_ADDR(CC2420_RXFIFO); \
        for (spiCnt = 0; ((spiCnt < (c)) && (FIFO_IS_1())); spiCnt++) { \
            FASTSPI_RX_GARBAGE(); \
        } \
        SPI_DISABLE(); \
    } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  FAST SPI: CC2420 RAM access (big or little-endian order)
//      p = pointer to the variable to be written
//      a = the CC2420 RAM address
//      c = the number of bytes to write
//      n = counter variable which is used in for/while loops (UINT8)
//
//  Example of usage:
//      UINT8 n;
//      UINT16 shortAddress = 0xBEEF;
//      FASTSPI_WRITE_RAM_LE(&shortAddress, CC2420RAM_SHORTADDR, 2);

#define FASTSPI_WRITE_RAM_LE(p,a,c,n) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX(0x80 | (a & 0x7F)); \
        FASTSPI_TX((a >> 1) & 0xC0); \
        for (n = 0; n < (c); n++) { \
            FASTSPI_TX(((UBYTE*)(p))[n]); \
        } \
        FASTSPI_TX(DUMMY); /*NOP*/\
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_RAM_LE(p,a,c,n) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX(0x80 | (a & 0x7F)); \
        FASTSPI_TX(((a >> 1) & 0xC0) | 0x20); \
        for (n = 0; n < (c); n++) { \
            FASTSPI_RX(((UBYTE*)(p))[n]); \
        } \
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_WRITE_RAM(p,a,c,n) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX(0x80 | (a & 0x7F)); \
        FASTSPI_TX((a >> 1) & 0xC0); \
        n = c; \
        do { \
            FASTSPI_TX(((UBYTE*)(p))[--n]); \
        } while (n); \
        FASTSPI_TX(DUMMY); /*NOP*/\
        SPI_DISABLE(); \
    } while (0)

#define FASTSPI_READ_RAM(p,a,c,n) \
    do { \
        SPI_ENABLE(); \
        FASTSPI_TX(0x80 | (a & 0x7F)); \
        FASTSPI_TX(((a >> 1) & 0xC0) | 0x20); \
        n = c; \
        do { \
            FASTSPI_RX(((UBYTE*)(p))[--n]); \
        } while (n); \
        SPI_DISABLE(); \
    } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Other useful SPI macros
#define FASTSPI_RESET_CC2420() \
    do { \
        FASTSPI_SETREG(CC2420_MAIN, 0x0000); \
        FASTSPI_SETREG(CC2420_MAIN, 0xF800); \
    } while (0)
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                    INTERRUPTS                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// General
#define ENABLE_GLOBAL_INT()         irq_en();
#define DISABLE_GLOBAL_INT()        irq_dis();
//-------------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------------
// Timer/Counter 1 interrupts (TIMER1 of the OKI processor)

// Compare match interrupt A (TIMER1 of the OKI)
#define ENABLE_T1_COMPA_INT()       do { put_hvalue(TIMECNTL1, get_hvalue(TIMECNTL1) | 0x0008); put_hvalue(TIMECNTL2, get_hvalue(TIMECNTL2) | 0x0008); } while (0)
#define DISABLE_T1_COMPA_INT()      do { put_hvalue(TIMECNTL1, get_hvalue(TIMECNTL1) & 0xFFF7); put_hvalue(TIMECNTL2, get_hvalue(TIMECNTL2) & 0xFFF7); } while (0)
#define CLEAR_T1_COMPA_INT()        do { put_hvalue(TIMESTAT1, 0x0001); } while (0)

// Compare match interrupt C (TIMER3 of the OKI)
#define ENABLE_T1_COMPC_INT()       do { put_hvalue(TIMECNTL3, get_hvalue(TIMECNTL3) | 0x0019);  } while (0)
#define DISABLE_T1_COMPC_INT()      do { put_hvalue(TIMECNTL3, get_hvalue(TIMECNTL3) & 0xFFE6); } while (0)
#define CLEAR_T1_COMPC_INT()        do { put_hvalue(TIMESTAT3, 0x0001);  } while (0)

// Capture interrupt
#define ENABLE_T1_CAPTURE_INT()     do { ENABLE_SFD_CAPTURE_INT(); } while (0)
#define DISABLE_T1_CAPTURE_INT()    do { DISABLE_SFD_CAPTURE_INT(); } while (0)
#define CLEAR_T1_CAPTURE_INT()      do { CLEAR_SFD_CAPTURE_INT(); } while (0)
//-------------------------------------------------------------------------------------------------------



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                      TIMER 1                      **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Timer 1 (16 bits)

// Initialize timer 1 for interrupt at regular intervals, controlled by the compare A value, and the
// precaler value
#define TIMER1_INIT(options) \
	do { \
		put_hvalue(TIMECNTL1, 0x0000); \
		put_hvalue(TIMECNTL2, 0x0000); \
		put_hvalue(TIMECNTL3, 0x0000); \
		put_hvalue(TIMECNTL3, 0x0001); /*One shot timer*/ \
		put_hvalue(TIMESTAT1, 0x0001); \
		put_hvalue(TIMESTAT2, 0x0001); \
		put_hvalue(TIMESTAT3, 0x0001); \
		IRQ_HANDLER_TABLE[INT_TIMER1] = timerHandler; \
		IRQ_HANDLER_TABLE[INT_TIMER2] = nullHandler; \
		IRQ_HANDLER_TABLE[INT_TIMER3] = timerHandlerC; \
		set_wbit(ILC, ILC_ILC16 & ILC_INT_LV1); \
		set_wbit(ILC, ILC_ILC18 & ILC_INT_LV1); \
		put_hvalue(TIMEBASE1, 0x0000); \
		put_hvalue(TIMEBASE2, 0x0000); \
		put_hvalue(TIMEBASE3, 0x0000); \
		put_hvalue(TIMECMP1, 0x4987); \
		put_hvalue(TIMECMP2, 0x1262); \
		put_hvalue(TIMECMP3, 0x00C8); \
		put_hvalue(TIMECNTL1, 0x0010); \
		put_hvalue(TIMECNTL2, 0x0010); \
		/* put_hvalue(TIMECNTL3, 0x0010); Do not start the timer! */ \
		put_hvalue(TIMECNTL1, 0x0018); \
		put_hvalue(TIMECNTL2, 0x0018); \
		/* TCCR1B = (TCCR1B & 0x07) | options | BM(WGM12); */ \
	} while (0)

// Options for TIMER1_INIT
#define TIMER1_CAPTURE_NOISE_CANCELLER  2
#define TIMER1_CAPTURE_ON_POSEDGE       1
#define TIMER1_CAPTURE_ON_NEGEDGE       0

// Start/stop
#define TIMER1_START(prescaler)         do { put_hvalue(TIMECNTL1, get_hvalue(TIMECNTL1) | 0x0018); put_hvalue(TIMECNTL2, get_hvalue(TIMECNTL2) | 0x0018); } while (0)
#define TIMER1_STOP()                   do { put_hvalue(TIMECNTL1, get_hvalue(TIMECNTL1) & 0xFFF7); put_hvalue(TIMECNTL2, get_hvalue(TIMECNTL2) & 0xFFF7); } while (0)

// Prescaler definitions to be used with TIMER1_START(...)
#define TIMER1_CLK_STOP                 0x00 /* Stop mode (the timer is not counting)    */
#define TIMER1_CLK_DIV1                 0x01 /* Total period = Clock freq / 256          */
#define TIMER1_CLK_DIV8                 0x02 /* Total period = Clock freq / (256 * 8)    */
#define TIMER1_CLK_DIV64                0x03 /* Total period = Clock freq / (256 * 32)   */
#define TIMER1_CLK_DIV256               0x04 /* Total period = Clock freq / (256 * 64)   */
#define TIMER1_CLK_DIV1024              0x05 /* Total period = Clock freq / (256 * 128)  */
#define TIMER1_CLK_T1_NEGEDGE           0x06 /* Total period = Clock freq / (256 * 256)  */
#define TIMER1_CLK_T1_POSEDGE           0x07 /* Total period = Clock freq / (256 * 1024) */

// Compare value modification
#define TIMER1_SET_COMPA_VALUE(value)   do { put_hvalue(TIMECMP1, value); } while (0)
#define TIMER1_SET_COMPB_VALUE(value)   do { put_hvalue(TIMECMP2, value); } while (0)
#define TIMER1_SET_COMPC_VALUE(value)   do { put_hvalue(TIMECMP3, value); } while (0)
//-------------------------------------------------------------------------------------------------------



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   USEFUL STUFF                    **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Useful stuff
#define NOP() asm volatile ("nop\n\t" ::)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  void halWait(UINT16 timeout)
//
//  DESCRIPTION:
//      Runs an idle loop for [timeout] microseconds.
//
//  ARGUMENTS:
//      UINT16 timeout
//          The timeout in microseconds
//-------------------------------------------------------------------------------------------------------
void halWait(UINT16 timeout);




/*******************************************************************************************************
 *******************************************************************************************************
 **************************              SIMPLE CC2420 FUNCTIONS              **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
//  Example of usage: Starts RX on channel 14 after reset
//      FASTSPI_RESET_CC2420();
//      FASTSPI_STROBE(CC2420_SXOSCON);
//      halRfSetChannel(14);
//      ... other registers can for instance be initialized here ...
//      halRfWaitForCrystalOscillator();
//      ... RAM access can be done here, since the crystal oscillator must be on and stable ...
//      FASTSPI_STROBE(CC2420_SRXON);
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  void rfWaitForCrystalOscillator(void)
//
//  DESCRIPTION:
//      Waits for the crystal oscillator to become stable. The flag is polled via the SPI status byte.
//
//      Note that this function will lock up if the SXOSCON command strobe has not been given before the
//      function call. Also note that global interrupts will always be enabled when this function
//      returns.
//-------------------------------------------------------------------------------------------------------
void halRfWaitForCrystalOscillator(void);


//-------------------------------------------------------------------------------------------------------
//  void halRfSetChannel(UINT8 Channel)
//
//  DESCRIPTION:
//      Programs CC2420 for a given IEEE 802.15.4 channel.
//      Note that SRXON, STXON or STXONCCA must be run for the new channel selection to take full effect.
//
//  PARAMETERS:
//      UINT8 channel
//          The channel number (11-26)
//-------------------------------------------------------------------------------------------------------
void halRfSetChannel(UINT8 channel);


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: hal.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:47  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:26  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:11:58  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.4  2004/10/27 04:31:04  simonhan
 * update xyz
 *
 * Revision 1.3  2004/10/27 03:00:52  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.7  2004/08/13 13:04:41  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
