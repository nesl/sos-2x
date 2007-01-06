#include <sos_sched.h>
#include <message.h>
#include <sos_timer.h>
#include <malloc.h>
#include <string.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include "hardware.h"
#include "cpld.h"
#include <sos_sched.h>
#include "adcm1700Const.h"

#define CPLD_TID               1

static uint8_t cpldState;
static cpldCommand_t *cpldCode;




static int8_t cpld_handler (void *state, Message * e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
 mod_id:CPLD_PID,
 state_size:0,
 num_timers:1,
 num_sub_func:0,
 num_prov_func:0,
 module_handler:cpld_handler,
};


//****************************Utility functions************************
void
delay ()
{
  asm volatile ("nop"::);
}

static inline void
clk ()
{
  delay ();
  CLEAR_CLOCK ();
  delay ();
  SET_CLOCK ();
  delay ();
  CLEAR_CLOCK ();
}


static inline void downloadCpld (cpldCommand_t * myCpld)
{
  // uP control of the address and data busses is released whenever a new CPLD command
  // is downloaded.
  uint8_t i;
  char buf[sizeof (cpldCommand_t)];
  //copy data structure to char memory so that outp does not complain about data mismatch.
  memcpy (buf, (char *) myCpld, sizeof (cpldCommand_t));
  //size of cpldCommand_t devide by eight (shift 3) is number of byte to put on bus
	
  SET_CPLD_PROGRAM_MODE ();	// place CPLD in program mode
  SET_RAM_LINES_PORT_MODE ();	// ... before taking control of data bus
  MAKE_PORTA_OUTPUT ();		// use data lines for programming
  MAKE_PORTC_INPUT_PULL_UP ();	// return high address lines to input mode (after direct memory access)
  for (i = 0; i < (sizeof (cpldCommand_t)); i++)
    {
      PORTA = buf[i];
      clk ();
    }
  ADDRESS_LATCH_OUTPUT_DISABLE ();	// release uP control of address bus, and
  MAKE_PORTA_INPUT_PULL_UP ();	// release uP control of data bus
  SET_CPLD_RUN_MODE ();		// ... before changing CPLD mode
}

static inline void setDirectMemAccess()		//This is to let CPU take control of SRAM and FLASH
{
  cpldCommand_t myCpldCommand;
  myCpldCommand.opcode = CPLD_OPCODE_MCU_ACCESS_SRAM;
  myCpldCommand.sramBank = 0;
  myCpldCommand.flashBank = 0;
  myCpldCommand.startPageAddress = 0x11;
  myCpldCommand.endPageAddress = 0xff;
	
  HAS_CRITICAL_SECTION;
	
  // This critical section ensures integrity at the hardware level...
  ENTER_CRITICAL_SECTION ();
  {
    cpldState = CPLD_MCU_ACCESS_SRAM;
  }
  LEAVE_CRITICAL_SECTION ();
	
  downloadCpld (&myCpldCommand);
  ADDRESS_LATCH_OUTPUT_ENABLE ();	// uP takes control of low address bus
  MAKE_PORTC_OUTPUT ();		        // uP takes control of high address bus
  MAKE_PORTA_OUTPUT ();		        // uP takes control of data bus
  SET_RAM_LINES_EXTERNAL_SRAM_MODE ();
	
	
  // This critical section ensures integrity at the hardware level...
  ENTER_CRITICAL_SECTION ();
  {
    SCHED_RESUME ();
  }
  LEAVE_CRITICAL_SECTION ();
}

static inline void setCpldStanddBy()
{
  cpldCommand_t myCpldCommand;
  myCpldCommand.opcode = CPLD_OPCODE_STANDBY;
  myCpldCommand.sramBank = 0;
  myCpldCommand.flashBank = 0;
  myCpldCommand.startPageAddress = 0;
  myCpldCommand.endPageAddress = 0;
  downloadCpld (&myCpldCommand);
  HAS_CRITICAL_SECTION;
	
  // This critical section ensures integrity at the hardware level...
  ENTER_CRITICAL_SECTION ();
  {
    cpldState = CPLD_STANDBY;
  }
  LEAVE_CRITICAL_SECTION ();
	
	
  // Drive all of the address and data lines to zero in order to minimize power consumption
  // by avoiding floating lines without resorting to pull-up resistors.
  ADDRESS_LATCH_OUTPUT_ENABLE (); // uP takes control of low address bus
  MAKE_PORTA_ZERO ();		  // uP takes control of high address bus
  MAKE_PORTC_ZERO ();		  // uP takes control of data bus
  SET_ADDRESS_LATCH ();
  TOSH_uwait (5);
  CLEAR_ADDRESS_LATCH ();
}


//****************************Initialization and Termination ************************
int8_t cpldInit ()
{
  HAS_CRITICAL_SECTION;
  uint8_t s;
	
  // This critical section ensures integrity at the hardware level...
  ENTER_CRITICAL_SECTION ();
  {
    cpldState = CPLD_OFF;
  }
  LEAVE_CRITICAL_SECTION ();
	
  // This if is here to avoid a component suddendy change the status of the device
  // if it has been already started and in bussiness by sombody else
  // This critical section ensures integrity at the hardware level...
  ENTER_CRITICAL_SECTION ();
  {
    s = cpldState;
  }
  LEAVE_CRITICAL_SECTION ();
	
  if (s == CPLD_OFF){
    INT_DISABLE ();
    SET_CPLD_RUN_MODE ();	// start from run mode to ensure correct programming            
    MAKE_PORTA_INPUT_PULL_UP ();
    MAKE_PORTC_INPUT_PULL_UP ();
    // A 100 usec delay is required in order to allow the 1.8 V regulator to
    // reach steady state. 
    TURN_VOLTAGE_REGULATOR_ON ();
    TOSH_uwait (100);
    SET_RAM_CONTROL_LINES_ONE ();
    // A 500 usec delay is required to allow the clock to start.
    TURN_CPLD_CLOCK_ON ();
    TOSH_uwait (500);
    //rlb: Capture can't be performed directly from standby mode because HS2 is already
    //     low. It causes subtle problems that interfere with the correct operation
    //     of the imager I2C interface.                             
    setDirectMemAccess ();	//alternative: setCpldStanddBy();
  }
	
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t cpld_handler (void *state, Message * msg)
{
  switch (msg->type){
  case MSG_INIT:
    {
      break;
    }
  case MSG_FINAL:
    {
      setCpldStanddBy ();
      delay ();
      SET_CPLD_PROGRAM_MODE ();	// set all handshake lines low
      CLEAR_CLOCK ();
      TURN_MCLK_OFF ();
      TURN_CPLD_CLOCK_OFF ();
      // Kevin - fix these I2C functions
      SET_I2C_CLOCK_OFF ();	// prevent I2C lines from powering CPLD
      SET_I2C_DATA_OFF ();
      ADDRESS_LATCH_OUTPUT_DISABLE ();
      SET_RAM_LINES_PORT_MODE ();	// prevent uP from driving unpowered CPLD
      SET_RAM_CONTROL_LINES_ZERO ();	// prevent RD,WR from driving unpowered CPLD                   
      // Drive address and data ports low to keep inputs from floating (and consuming high current)
      MAKE_PORTA_ZERO ();
      MAKE_PORTC_ZERO ();
      TURN_VOLTAGE_REGULATOR_OFF ();
      HAS_CRITICAL_SECTION;
      // This critical section ensures integrity at the hardware level...
      ENTER_CRITICAL_SECTION ();
      {
	cpldState = CPLD_OFF;
      }
      LEAVE_CRITICAL_SECTION ();
      break;
    }
  case MSG_TIMER_TIMEOUT:
    {
      switch (cpldCode->opcode & 0x0f){	// signal after delay
      case CPLD_OPCODE_RUN_CAMERA:
	post_long (ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE,
		   sizeof (cpldCode), (void *) cpldCode, 0);
	break;
      default:
	break;
      }
      break;
    }
  case PROCESS_SIGNAL:
    {
      INT_DISABLE ();		// disable further interrupts                                
      post_long (ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE,
		 sizeof (cpldCode), (void *) cpldCode, 0);
      break;
    }
  case SET_CPLD_MODE_DONE:
    {
      break;
    }
  default:
    return -EINVAL;
  }
  return SOS_OK;
}

// *** NOTE: 1) release control of the data and address busses BEFORE downloading a new CPLD command
//           2) take control of the data and address busses AFTER downloading a new CPLD command

//****************************Setting CPLD Status and such****************************
int8_t setCpldMode (cpldCommand_t * myCpld)
{
  //if(state != CPLD_STANDBY) return -EINVAL;
  //     SET_RAM_LINES_PORT_MODE();
  cpldCode = myCpld;
  HAS_CRITICAL_SECTION;
  //these cases the result are immidiate. The rest of the casese should wait for Interrupt.
  // ***************************************************************
  // ******* FIX THIS !!!   FIX THIS !!! ***************************
  // Unless interrupts are enabled at this point, an interrupt will 
  // occur when capture is intiated, regardless of the state of HS2. 
  // This could be caused by a glitch in HS2, which is not registered,
  // but I don't see it on the logic analyzer, or the scope. 
  //       INT_DISABLE();   // enable interrupts first, or it doesn't work.   
  INT_ENABLE ();		// enable interrupts first, or it doesn't work.   
  // ***************************************************************
  // ***************************************************************
  switch ((cpldCode->opcode & 0x0f))	// RLB 7/21/05: filtered out repeat count
    {
    case CPLD_OPCODE_STANDBY:
    case CPLD_OPCODE_RESET:
      {
	downloadCpld (cpldCode);
	// Drive all of the address and data lines to zero in order to minimize power consumption
	// by avoiding floating lines without resorting to pull-up resistors.
	ADDRESS_LATCH_OUTPUT_ENABLE ();	// uP takes control of low address bus
	MAKE_PORTA_ZERO ();	// uP takes control of high address bus
	MAKE_PORTC_ZERO ();	// uP takes control of data bus
	SET_ADDRESS_LATCH ();
	TOSH_uwait (5);
	CLEAR_ADDRESS_LATCH ();
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_STANDBY;
	}
	LEAVE_CRITICAL_SECTION ();
	post_long (ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE,
		   sizeof (cpldCode), (void *) cpldCode, 0);
	break;
      }
    case CPLD_OPCODE_MCU_ACCESS_SRAM_TEST:
      {
	//mhr::This is the default mode
	downloadCpld (cpldCode);
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_MCU_ACCESS_SRAM_TEST;
	}
	LEAVE_CRITICAL_SECTION ();
	ADDRESS_LATCH_OUTPUT_ENABLE ();	// uP takes control of low address bus
	MAKE_PORTC_OUTPUT ();	// uP takes control of high address bus
	MAKE_PORTA_OUTPUT ();	// uP takes control of data bus
	SET_RAM_LINES_EXTERNAL_SRAM_MODE ();
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  SCHED_RESUME ();
	}
	LEAVE_CRITICAL_SECTION ();
	//atomic {SCHED_RESUME();}             
	//      post_long(ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE, sizeof(cpldCode), (void*)cpldCode, 0);
	// signal cpld.setCpldModeDone(cpldCode); //This assumes that CPLD does it very fast
	break;
      }
			
    case CPLD_OPCODE_MCU_ACCESS_SRAM:
      {
	//mhr::This is the default mode
	downloadCpld (cpldCode);
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_MCU_ACCESS_SRAM;
	}
	LEAVE_CRITICAL_SECTION ();
	ADDRESS_LATCH_OUTPUT_ENABLE ();	// uP takes control of low address bus
	MAKE_PORTC_OUTPUT ();	// uP takes control of high address bus
	MAKE_PORTA_OUTPUT ();	// uP takes control of data bus
	SET_RAM_LINES_EXTERNAL_SRAM_MODE ();
			
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  SCHED_RESUME ();
	}
	LEAVE_CRITICAL_SECTION ();
	post_long (ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE,
		   sizeof (cpldCode), (void *) cpldCode, 0);
	break;
      }
			
    case CPLD_OPCODE_MCU_ACCESS_FLASH:
      {
	//mhr:: This is something to think about it
	downloadCpld (cpldCode);
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_MCU_ACCESS_FLASH;
	}
	LEAVE_CRITICAL_SECTION ();
	// atomic { state = CPLD_MCU_ACCESS_FLASH; }
	ADDRESS_LATCH_OUTPUT_ENABLE ();	// uP takes control of low address bus
	MAKE_PORTC_OUTPUT ();	// uP takes control of high address bus
	MAKE_PORTA_OUTPUT ();	// uP takes control of data bus
	SET_RAM_LINES_EXTERNAL_SRAM_MODE ();
	post_long (ADCM1700_CONTROL_PID, CPLD_PID, SET_CPLD_MODE_DONE,
		   sizeof (cpldCode), (void *) cpldCode, 0);
	break;
      }
			
    case CPLD_OPCODE_CAPTURE_IMAGE:	//returns through INT
      {
	//mhr:: DMA mode
	//rlb: Capture should not be attempted from STANDBY mode.
	INT_ENABLE ();		// enable interrupts first, or it doesn't work.                   
	downloadCpld (cpldCode);
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_CAPTURE_IMAGE;
	}
	LEAVE_CRITICAL_SECTION ();
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  SCHED_STALL ();
	}
	LEAVE_CRITICAL_SECTION ();
	break;
      }
			
    case CPLD_OPCODE_CAPTURE_TEST_PATTERN:	//returns through INT
      {
	//mhr:: DMA mode
	downloadCpld (cpldCode);
	INT_ENABLE ();
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_CAPTURE_TEST_PATTERN;
	}
	LEAVE_CRITICAL_SECTION ();
	ENTER_CRITICAL_SECTION ();
	{
	  SCHED_STALL ();
	}
	LEAVE_CRITICAL_SECTION ();
			
	//atomic { state = CPLD_CAPTURE_TEST_PATTERN; }
	//atomic {SCHED_STALL();}
	break;
      }
			
    case CPLD_OPCODE_TRANSFER_SRAM_TO_FLASH:	//returns through INT
      {
	// RLB: 4/29/05
	// The CPLD must wait for at least 20 milliseconds after each segment write. The uP provides a
	// 30 msec clock signal on HS1 for this purpose. In the original design, the CPLD was to use an 
	// RC delay. Unfortunately the time constant is too short with the existing RC values, and the 
	// slowly slewing signal could cause excessive current consumption when applied to a digital input.  
	downloadCpld (cpldCode);
	INT_ENABLE ();
	//call Timer.start(TIMER_REPEAT, 15);    // 30 millisecond period for Flash segment writes (on HS1)                                   
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_TRANSFER_SRAM_TO_FLASH;
	}
	LEAVE_CRITICAL_SECTION ();
	break;
      }
			
    case CPLD_OPCODE_TRANSFER_FLASH_TO_SRAM:	//returns through INT
      {
	//FIX ME!
	//mhr:: This is something to think about it: SCHED_STALL();
	downloadCpld (cpldCode);
	INT_ENABLE ();
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_TRANSFER_FLASH_TO_SRAM;
	}
	LEAVE_CRITICAL_SECTION ();
	break;
      }
			
    case CPLD_OPCODE_RUN_CAMERA:
      {
	// RLB: 4/29/05 
	// This mode is obsolete in Cyclops2. The camera clock is now controlled independently by HS3,
	// not by the CPLD. This mode has only been included for backward compatibility.
	// The uP may access the SRAM in this mode.
	downloadCpld (cpldCode);
			
	// This critical section ensures integrity at the hardware level...
	ENTER_CRITICAL_SECTION ();
	{
	  cpldState = CPLD_RUN_CAMERA;
	}
	LEAVE_CRITICAL_SECTION ();
	ker_timer_init (CPLD_PID, CPLD_TID, TIMER_ONE_SHOT);
	ker_timer_start (CPLD_PID, CPLD_TID, 150);
	break;
      }
    default:
      return -EINVAL;
    }
  return SOS_OK;
}

int8_t stdByCpld ()
{
  setCpldStanddBy ();
  return SOS_OK;
}

//**************************************************** 
//A transaction such as Image Capture or Copy between Flash and SRAM has been
//done. We post a task so that the result in upper layer does not happen inside
//an interrupt service routine.
SIGNAL (SIG_INTERRUPT7)
{
	
  LED_DBG(LED_RED_ON);
  uint8_t test = 0;		// debugigng variable in order to ease jtag debugging.
  switch (cpldState)
    {
    case CPLD_TRANSFER_SRAM_TO_FLASH:
      test++;
      //                  call Timer.stop();     // stop HS1 delay timer for Flash write
      CLEAR_CLOCK ();
    case CPLD_CAPTURE_IMAGE:
      test++;
    case CPLD_CAPTURE_TEST_PATTERN:
      test++;
    case CPLD_TRANSFER_FLASH_TO_SRAM:
      test++;
      setDirectMemAccess ();	// Kevin - misinterpretation, its not DMA from CPLD and memory but from CPU and memory
      post_short (CPLD_PID, CPLD_PID, PROCESS_SIGNAL, 0, 0, 0);
      break;
    default:
      break;
    }
  //post processSignal();
  return;
}
