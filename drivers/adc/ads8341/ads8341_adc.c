/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief ADC module for the ADS8341
 * @author Naim Busek {ndbusek@gmail.com
 */

/** 
 * Module needs to include <module.h>
 */
#include <sos.h>
#include <hardware.h>

#include <sos_timer.h>

#include <ads8341_adc_hal.h>

#include "ads8341_adc.h"

#define WRITE_LEN 1
#define READ_LEN 3

#define ADS8341_BITS 16
#define ADS8341_BIT_ERROR 1

#define NULL_PORT 0xff
#define FORCE_STOP 0xfe

// min and max sampeling intervals
#define MIN_INTERVAL 10
#define MAX_INTERVAL 250

// this is limited by the counter
// floor(0xff/READ_LEN) = 85
#define MAX_SAMPLE_CNT 85

// sampling type
#define ADS8341_ADC_DMA_FLAG				0x10
#define ADS8341_ADC_FAST_FLAG				0x08
#define ADS8341_ADC_PERODIC_FLAG		0x04
#define ADS8341_ADC_CALIBRATED_FLAG	0x02
#define ADS8341_ADC_SINGLE_FLAG			0x01

//#define NO_BUSY_INTR

/**
 * Varrious states that the module transitions through
 */
enum {
	ADS8341_ADC_INIT=0,  // system uninitalized
	ADS8341_ADC_INIT_BUSY,  // system busy with initalization
	ADS8341_ADC_BACKOFF,  // init failed init msg to self and waiting
	ADS8341_ADC_IDLE,    // system initalized and idle
	ADS8341_ADC_TX,      // send command pending to adc
	ADS8341_ADC_TX_RX,   // tx done rx pending waiting for busy_done interrupt
	ADS8341_ADC_RX,      // rx pending (after busy_done interrupt or send done msg)
	ADS8341_ADC_DONE,    // rx done data ready
	ADS8341_ADC_WAIT,    // perodic sampeling sample done waiting for clock
	ADS8341_ADC_ERROR,   // error state
};


/**
 * System status flags
 */
#define ADS8341_ADC_HW_IDLE      0x80
#define ADS8341_ADC_HW_SEND_DONE 0x40
#define ADS8341_ADC_HW_READ_DONE 0x20

/**
 * Module state
 */
typedef struct ads8341_adc_state {
	uint8_t state;
	uint8_t state_flags;
	
	uint8_t calling_mod_id;

	uint8_t spi_cmd;
	spi_addr_t addr;
	
	uint8_t portmap[ADS8341_ADC_PORTMAPSIZE];
	uint8_t calling_pid[ADS8341_ADC_PORTMAPSIZE];
	adc16_cb_t cb[ADS8341_ADC_PORTMAPSIZE];
	
	uint8_t preamp_gain;
	uint8_t reqPort;

	uint8_t readBuf[READ_LEN];
	
	uint8_t flags;
	
	uint8_t *dataBuf;
	uint16_t cnt;
	uint16_t idx;
} ads8341_adc_state_t;

static ads8341_adc_state_t s;

static int8_t ads8341_adc_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = ADS8341_ADC_PID,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = ads8341_adc_msg_handler,
	/*
	.funct {
		{error_8, "cvv0", RUNTIME_PID, RUNTIME_FID},
		{error_16, "lvv0", RUNTIME_PID, RUNTIME_FID},
		{error_16, "lvv0", RUNTIME_PID, RUNTIME_FID},
		{error_16, "lvv0", RUNTIME_PID, RUNTIME_FID},
		{error_16, "lvv0", RUNTIME_PID, RUNTIME_FID},
	}*/
};


int8_t ads8341_adc_send_done(
		uint8_t bytes,
		uint8_t count,
		uint8_t status);

int8_t ads8341_adc_read_done(
		uint8_t *buff,
		uint8_t bytes,
		uint8_t count,
		uint8_t status);

static inline void read_ads8341(void);

int8_t ads8341_adc_init() {

	s.state = ADS8341_ADC_INIT;

	ads8341_adc_hardware_init();

	s.addr.cs_reg = ADC_CS_PORT();
	s.addr.cs_bit = ADC_CS_BIT();
	
	ker_register_module(sos_get_header_address(mod_header));
	
	return SOS_OK;
}


// WARNING: alters system state
// should only be called from within a critical section
void ads8341_adc_reset() {
	s.calling_mod_id = NULL_PID;
	s.reqPort = NULL_PORT;
	s.flags = 0;
	s.state = ADS8341_ADC_IDLE;
	s.idx = 0;
}


int8_t ads8341_adc_msg_handler(void *state, Message *msg) { 
	HAS_CRITICAL_SECTION;
	
	switch (msg->type) {
		case MSG_INIT:
			// initalize adc with a single conversion 
			ads8341_adc_on(); // disable force shutdown (will enable autoshutdown later)
			ker_vref(VREF_ON);

			// this MUST be initalized before the init sample request
			s.reqPort = NULL_PORT;
			s.flags = 0;

			if (ker_ads8341_adc_getData(0) != SOS_OK) {
				s.state = ADS8341_ADC_BACKOFF;
				// reset the system and reschedule init at end of system queue
				s.reqPort = NULL_PORT;
				s.flags = 0;
				post_short(ADS8341_ADC_PID, ADS8341_ADC_PID, MSG_INIT, 0, 0, SOS_MSG_SYSTEM_PRIORITY);
			}

			ENTER_CRITICAL_SECTION();
			s.calling_mod_id = NULL_PID;
			s.state = ADS8341_ADC_INIT_BUSY;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_FINAL: 
			ads8341_adc_off();
			ker_vref(VREF_OFF);
			break;

		case MSG_SPI_SEND_DONE:
			/* only used if not interrupt driven */
#ifdef NO_BUSY_INTR
			/*
				 if (ker_spi_reserve_bus(ADS8341_ADC_PID, s.addr, SPI_SYS_NULL_FLAG) != SOS_OK) {
				 ker_spi_release_bus(ADS8341_ADC_PID);
				 ENTER_CRITICAL_SECTION();
				 s.state = ADS8341_ADC_ERROR;
				 ads8341_adc_reset();
				 LEAVE_CRITICAL_SECTION();
				 post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
				 break;
				 }

				 if (ker_spi_read_data(NULL, READ_LEN, 1, ADS8341_ADC_PID) != SOS_OK) {
				 ker_spi_release_bus(ADS8341_ADC_PID);
					ENTER_CRITICAL_SECTION();
					s.state = ADS8341_ADC_ERROR;
					ads8341_adc_reset();
					LEAVE_CRITICAL_SECTION();
					post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
					break;  
				}
				ENTER_CRITICAL_SECTION();
				if (s.state != ADS8341_ADC_INIT_BUSY) {
				s.state = ADS8341_ADC_RX;
				}
				LEAVE_CRITICAL_SECTION();
				*/
#endif
			break;

		case MSG_SPI_READ_DONE:
			{
				uint16_t adcValue = 0;

				s.state_flags |= ADS8341_ADC_HW_READ_DONE;

				if ((s.state != ADS8341_ADC_INIT_BUSY) && (s.calling_mod_id != NULL_PID)){
					// copy out the spi data
					//adcValue = ((s.readBuf[0]<<9)&0xfe00) + ((s.readBuf[1]<<1)&0x01fe) + ((s.readBuf[2]&0x80) ? 1 : 0);
					adcValue = ((((uint16_t)msg->data[0])<<9)&0xfe00) + (((uint16_t)msg->data[1])<<1&0x01fe) + ((((uint16_t)msg->data[2])&0x80) ? 1 : 0);
					post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_DATA_READY, s.portmap[s.reqPort], adcValue, SOS_MSG_HIGH_PRIORITY);
				}

				ENTER_CRITICAL_SECTION();
				if ((s.flags & ADS8341_ADC_PERODIC_FLAG) && (s.idx < s.cnt)) {
					// and wait for timer
					s.state = ADS8341_ADC_WAIT;
				} else {
					ads8341_adc_reset();
				}
				LEAVE_CRITICAL_SECTION();
			}
			break;

		case MSG_ERROR:
			ENTER_CRITICAL_SECTION();
			s.state = ADS8341_ADC_ERROR;
			post_short(ADS8341_ADC_PID, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_SYSTEM_PRIORITY);
			LEAVE_CRITICAL_SECTION();
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t ker_ads8341_adc_bindPort(uint8_t port, uint8_t adcChannel, uint8_t driverpid, adc16_cb_t cb){
	HAS_CRITICAL_SECTION;
	//used for remapping as a result of hardware mistakes
	ENTER_CRITICAL_SECTION();
	s.portmap[port] = adcChannel;
	s.calling_pid[port] = driverpid;
	if (cb != NULL) {
		s.cb[port] = cb;
	} else {
		s.cb[port] = NULL;
	}
	LEAVE_CRITICAL_SECTION();
	
	return SOS_OK;
}


int8_t ker_ads8341_adc_getData(uint8_t port){
	uint8_t portMask = 0;
	uint8_t spi_flags = 0;
	HAS_CRITICAL_SECTION;
	
	switch (s.state) {
		case ADS8341_ADC_INIT_BUSY:
		case ADS8341_ADC_TX:
			return -EBUSY;
			
		case ADS8341_ADC_BACKOFF:
		case ADS8341_ADC_INIT:
			s.state = ADS8341_ADC_INIT_BUSY;
			// fall through

		case ADS8341_ADC_IDLE:
			if ((s.flags & ADS8341_ADC_PERODIC_FLAG) == 0) {
				if (s.reqPort != NULL_PORT) {
					return -EBUSY;
				}
				if (s.flags == 0x00) {
					s.flags = ADS8341_ADC_SINGLE_FLAG;
				}
			} else {
				if (s.reqPort != port) {
					return -EBUSY;
				}
			}

			spi_flags = SPI_SYS_READ_PENDING_FLAG | SPI_SYS_SEND_DONE_CB_FLAG | SPI_SYS_SHARED_MEM_FLAG;  // always set
			//spi_flags = SPI_SYS_READ_PENDING_FLAG | SPI_SYS_SEND_DONE_CB_FLAG;  // always set
			if (s.flags & ADS8341_ADC_PERODIC_FLAG) {   // only set if doing perodic reads
				spi_flags |= SPI_SYS_LOCK_BUS_FLAG;
			}
			if (s.cb[port] != NULL) {
				spi_flags |= SPI_SYS_READ_DONE_CB_FLAG;
			}

			if (ker_spi_reserve_bus(ADS8341_ADC_PID, s.addr, spi_flags) != SOS_OK) {
				ker_spi_release_bus(ADS8341_ADC_PID);
				return -EBUSY;
			}
			if (ker_spi_register_send_cb(ADS8341_ADC_PID, &ads8341_adc_send_done) != SOS_OK) {
				ker_spi_release_bus(ADS8341_ADC_PID);
				return -EIO;
			}
			if (s.cb[port] != NULL) {
				if (ker_spi_register_read_cb(ADS8341_ADC_PID, &ads8341_adc_read_done) != SOS_OK) {
					ker_spi_release_bus(ADS8341_ADC_PID);
					return -EIO;
				}
			}

			// error checking
			if (port > ADS8341_ADC_PORTMAPSIZE) {
				return -EINVAL;
			}
			portMask = (1<<port);

			s.reqPort = port;
			s.calling_mod_id = s.calling_pid[port];

			switch (port) {
				case ADS8341_ADC_PORT0: { s.spi_cmd	= ADS8341_CH0; break; }
				case ADS8341_ADC_PORT1: { s.spi_cmd	= ADS8341_CH1; break; }
				case ADS8341_ADC_PORT2: { s.spi_cmd	= ADS8341_CH2; break; }
				case ADS8341_ADC_PORT3: { s.spi_cmd	= ADS8341_CH3; break; }
				default: { s.spi_cmd	= ADS8341_CH0; break; }
			}
			s.spi_cmd	|= (ADS8341_S | ADS8341_SD | ADS8341_PD1);
			// fall through

		case ADS8341_ADC_WAIT:
			s.state_flags &= ~ADS8341_ADC_HW_SEND_DONE;

			if (ker_spi_send_data( &(s.spi_cmd), WRITE_LEN, ADS8341_ADC_PID) != SOS_OK) {
				ENTER_CRITICAL_SECTION();
				s.state = ADS8341_ADC_ERROR;
				ads8341_adc_reset();
				LEAVE_CRITICAL_SECTION();
				post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
				break;
			}
			s.state_flags &= ~ADS8341_ADC_HW_IDLE;

			ENTER_CRITICAL_SECTION();
			// enable interrupt 4
			ads8341_adc_enable_interrupt();
			if (s.state != ADS8341_ADC_INIT_BUSY) {
				s.state = ADS8341_ADC_TX;
			}
			LEAVE_CRITICAL_SECTION();
			break;

		case ADS8341_ADC_ERROR:
		default:
			ENTER_CRITICAL_SECTION();
			ads8341_adc_reset();
			ker_spi_release_bus(ADS8341_ADC_PID);
			LEAVE_CRITICAL_SECTION();
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t ker_ads8341_adc_getPerodicData(uint8_t port, uint8_t prescaler, uint8_t interval, uint16_t count){
	/* 7.3728 MHz/1024 = 7.200KHz (prescaler) */
	/* 7200Hz/interval = rate Hz */
	/* interval = 7200/rate */
	HAS_CRITICAL_SECTION;
	
	if (s.reqPort != NULL_PORT) {
		return -EBUSY;
	}

	if ((interval < MIN_INTERVAL) || (interval > MAX_INTERVAL)) {
		return -EINVAL;
	} else {
		OCR2 = interval;
	}

	ENTER_CRITICAL_SECTION();
	s.cnt = count;
	s.idx = 0;
	s.reqPort = port;
	s.flags = ADS8341_ADC_PERODIC_FLAG;
	/* start the clock let the interrupt take care of everything else */
	ads8341_adc_clk_enable_interrupt();
	ads8341_adc_clk_set(0x00);					// zero counter
	ads8341_adc_clk_set_prescale(prescaler);					// zero counter
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_ads8341_adc_getDataDMA(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t *sharedMemory, uint16_t count){
	/* 7.3728 MHz/1024 = 7.200KHz (prescaler) */
	/* 7200Hz/interval = rate Hz */
	/* interval = 7200/rate */
	HAS_CRITICAL_SECTION;
	
	if (s.reqPort != NULL_PORT) {
		return -EBUSY;
	}
	if ((interval < MIN_INTERVAL) || (interval > MAX_INTERVAL) || (NULL == sharedMemory) || (count > MAX_SAMPLE_CNT)) {
		return -EINVAL;
	}

	s.dataBuf = sharedMemory;
	s.cnt = count;
	s.idx = 0;
	s.reqPort = port;
	s.flags = (ADS8341_ADC_PERODIC_FLAG | ADS8341_ADC_DMA_FLAG);

	ENTER_CRITICAL_SECTION();
	/* start the clock let the interrupt take care of everything else */
	ads8341_adc_clk_set_prescale(prescaler);					// zero counter
	ads8341_adc_clk_set_interval(interval);
	ads8341_adc_clk_set(0x00);					// zero counter
	ads8341_adc_clk_enable_interrupt();
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_ads8341_adc_stopPerodicData(uint8_t port) {
	HAS_CRITICAL_SECTION;
	
	if ((port != FORCE_STOP) && (s.reqPort != port)) {
		return -EPERM;
	}

	ENTER_CRITICAL_SECTION();
	ads8341_adc_clk_disable_interrupt();

	ads8341_adc_clk_stop();
	ads8341_adc_reset();
	LEAVE_CRITICAL_SECTION();
	
	return SOS_OK;
}


int8_t ads8341_adc_send_done(
		uint8_t bytes,
		uint8_t count,
		uint8_t status) {
	s.state_flags |= ADS8341_ADC_HW_SEND_DONE;

	if (s.state_flags & ADS8341_ADC_HW_IDLE) {
		read_ads8341();
	}

	return SOS_OK;
}


int8_t ads8341_adc_read_done(
		uint8_t *buff,
		uint8_t bytes,
		uint8_t count,
		uint8_t status) {
	HAS_CRITICAL_SECTION;
	uint16_t adcValue;

	s.state_flags |= ADS8341_ADC_HW_READ_DONE;
	
	if ((s.state != ADS8341_ADC_INIT_BUSY) && (s.calling_mod_id != NULL_PID) && (bytes == READ_LEN)){
		// copy out the spi data
		adcValue = ((s.readBuf[0]<<9)&0xfe00) + ((s.readBuf[1]<<1)&0x01fe) + ((s.readBuf[2]&0x80) ? 1 : 0);
		//((buff[0]<<9)&0xfe00) + (buff[1]<<1&0x01fe) + ((buff[2]&0x80) ? 1 : 0);

		if (s.cb[s.reqPort] != NULL) {
			s.cb[s.reqPort](adcValue, status);
		} else {
			post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_DATA_READY, s.portmap[s.reqPort], adcValue, SOS_MSG_HIGH_PRIORITY);
		}
		
		ENTER_CRITICAL_SECTION();
		if ((s.flags & ADS8341_ADC_PERODIC_FLAG) && (s.idx < s.cnt)) {
			// and wait for timer
			s.state = ADS8341_ADC_WAIT;
		} else {
			ads8341_adc_reset();
		}
		
		LEAVE_CRITICAL_SECTION();
	} else {
		ker_spi_unregister_read_cb(ADS8341_ADC_PID);
	}

	return SOS_OK;
}


/** perodic data timer interrupt */
ads8341_adc_clk_interrupt() {
	
	if (((s.flags & ADS8341_ADC_PERODIC_FLAG) == 0) || (ker_ads8341_adc_getData(s.reqPort) != SOS_OK)) {
		ads8341_adc_clk_disable_interrupt();
		ads8341_adc_reset();
	}
	/*
	if (s.idx >= s.cnt) {
	?????
	}
	*/
}

static inline void read_ads8341(void) {
	
	s.state_flags &= ~ADS8341_ADC_HW_READ_DONE;

	if (SOS_OK !=  ker_spi_read_data(s.readBuf, READ_LEN, 1, ADS8341_ADC_PID)) {
	//if (SOS_OK !=  ker_spi_read_data(NULL, READ_LEN, 1, ADS8341_ADC_PID)) {
		s.state = ADS8341_ADC_ERROR;
		ads8341_adc_reset();
		post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
		return;  
	}

	s.idx++;

	if (s.state != ADS8341_ADC_INIT_BUSY) {
		s.state = ADS8341_ADC_RX;
	}
}


/** interrupt handler */
ads8341_adc_busy_interrupt() {

	ads8341_adc_disable_interrupt();

	s.state_flags |= ADS8341_ADC_HW_IDLE;
	
	if (!(s.state == ADS8341_ADC_TX) && !(s.state == ADS8341_ADC_INIT_BUSY)) {
		ads8341_adc_reset();
		post_short(s.calling_mod_id, ADS8341_ADC_PID, MSG_ERROR, 0, 0, SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
		return;
	}

	if (s.state_flags & ADS8341_ADC_HW_SEND_DONE) {
		read_ads8341();
	}
}

