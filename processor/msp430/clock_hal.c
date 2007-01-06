/* -*-C-*- */

/**
 * @file clock_hal.c
 * @brief Basic Clocking Sub-system for MSP430
 *        Ported from TinyOS-1.x
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#include <hardware_proc.h>
#include <clock_hal.h>

//----------------------------------------------------------
// CONSTANTS
enum
  {
    ACLK_CALIB_PERIOD = 8,
    ACLK_KHZ          = 32,
    TARGET_DCO_KHZ    = 4096,
    TARGET_DCO_DELTA  = (TARGET_DCO_KHZ/ACLK_KHZ)*ACLK_CALIB_PERIOD,
  };

//----------------------------------------------------------
// FUNCTION PROTOTYPES
static void set_dco_calib(int calib);
static uint16_t test_calib_busywait_delta(int calib);
static void busyCalibrateDCO();
static void dco_calibrate_init();


//----------------------------------------------------------
// INITIALIZE CLOCK SYSTEM
void clock_hal_init()
{
  dco_calibrate_init();
  // BCSCTL1
  // .XT2OFF = 1; disable the external oscillator for SCLK and MCLK
  // .XTS = 0; set low frequency mode for LXFT1
  // .DIVA = 0; set the divisor on ACLK to 1
  // .RSEL, do not modify
  BCSCTL1 = XT2OFF | (BCSCTL1 & (RSEL2|RSEL1|RSEL0));
  
  // BCSCTL2
  // .SELM = 0; select DCOCLK as source for MCLK
  // .DIVM = 0; set the divisor of MCLK to 1
  // .SELS = 0; select DCOCLK as source for SCLK
  // .DIVS = 2; set the divisor of SCLK to 4
  // .DCOR = 0; select internal resistor for DCO
  BCSCTL2 = DIVS1;

  // IE1.OFIE = 0; no interrupt for oscillator fault
  IE1 &= ~(OFIE);

  return;
}


//----------------------------------------------------------
// LOCAL STATIC FUNCTION
static void set_dco_calib(int calib)
{
  BCSCTL1 = (BCSCTL1 & ~0x07) | ((calib >> 8) & 0x07);
  DCOCTL = calib & 0xff;
}

static uint16_t test_calib_busywait_delta(int calib)
{
  int8_t aclk_count = 2;
  uint16_t dco_prev = 0;
  uint16_t dco_curr = 0;
  
  set_dco_calib( calib );
  while( aclk_count-- > 0 ){
    TBCCR0 = TBR + ACLK_CALIB_PERIOD; // set next interrupt
    TBCCTL0 &= ~CCIFG; // clear pending interrupt
    while( (TBCCTL0 & CCIFG) == 0 ); // busy wait
    dco_prev = dco_curr;
    dco_curr = TAR;
  }  
  return dco_curr - dco_prev;
}

// busyCalibrateDCO
// Should take about 9ms if ACLK_CALIB_PERIOD=8.
// DCOCTL and BCSCTL1 are calibrated when done.
static void busyCalibrateDCO()
{
  // --- variables ---
  int calib;
  int step;
  
  // --- setup ---
  TACTL = TASSEL1 | MC1; // source SMCLK, continuous mode, everything else 0
  TBCTL = TBSSEL0 | MC1;
  BCSCTL1 = XT2OFF | RSEL2;
  BCSCTL2 = 0;
  TBCCTL0 = CM0;
  
  // --- calibrate ---
  // Binary search for RSEL,DCO,DCOMOD.
  // It's okay that RSEL isn't monotonic.
  
  for( calib=0,step=0x800; step!=0; step>>=1 ){
    // if the step is not past the target, commit it
    if( test_calib_busywait_delta(calib|step) <= TARGET_DCO_DELTA )
      calib |= step;
  }
  
  // if DCOx is 7 (0x0e0 in calib), then the 5-bit MODx is not useable, set it to 0
  if( (calib & 0x0e0) == 0x0e0 )
    calib &= ~0x01f;
  
  set_dco_calib( calib );
}

//----------------------------------------------------------
// Initialize DCO Calibration
// Currently calibrated to run at 4 MHz
static void dco_calibrate_init()
{
  HAS_CRITICAL_SECTION;
  TACTL = TACLR;
  TAIV = 0;
  TBCTL = TBCLR;
  TBIV = 0;
  ENTER_CRITICAL_SECTION();
  {
    busyCalibrateDCO(); // Sets the values for BCSCTL1(RSELx), DCOCTL(DCOx|MODx)
    BCSCTL1 = XT2OFF | (BCSCTL1 & (RSEL2|RSEL1|RSEL0)); // DISABLE EXTERNAL OSCILLASOR
    BCSCTL2 = DIVS_DIV4; // SMCLK DIVIDED BY 4 - Set to 1 MHz
    IE1 &= ~(OFIE);// CLEAR NO INTERRUPT FOR OSCILLATOR FLAG
    // TIMERA INIT
    TAR = 0;
    TACTL = TASSEL1 | TAIE;
    // TIMERB INIT
    TBR = 0;
    TBCTL = TBSSEL0 | TBIE;
  }
  LEAVE_CRITICAL_SECTION();
}
