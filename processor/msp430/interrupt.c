#include <module.h>
#include <msp430x16x.h>

typedef void (*isr_func_t)();

static int8_t isr_controller(void *state, Message *msg);
static sos_module_t isr_module;

static func_cb_ptr isr[MAX_USER_INTERRUPTS];
static sos_pid_t pid[MAX_USER_INTERRUPTS];

static const mod_header_t mod_header SOS_MODULE_HEADER ={
  mod_id : PROC_INTERRUPT_PID,
  state_size : 0,
  num_timers : 0,
  num_sub_func : MAX_USER_INTERRUPTS,
  num_prov_func : 0,
  module_handler: isr_controller,
  funct: {
  	{error_8, "vvv0", RUNTIME_PID, RUNTIME_FID},
	},
};

static int8_t isr_controller(void *state, Message *msg) {
	return SOS_OK;
}

int8_t interrupt_init() {
	HAS_CRITICAL_SECTION;
	unsigned int i;

	ENTER_CRITICAL_SECTION();

	for (i = 0; i < MAX_USER_INTERRUPTS; i++) {
		pid[i] = NULL_PID;
	}

	// Configure User Int (Port 2, Pin 7)
	// Set I/O function
	P2SEL &= ~BV(7);
	// Set Input direction
	P2DIR &= ~BV(7);
	// Set high-to-low interrupt edge select
	P2IES |= BV(7);
	// Enable interrupt
	P2IE |= BV(7);
  	
	LEAVE_CRITICAL_SECTION();

	// Register the interrupt controller module
	return sched_register_kernel_module(&isr_module,
			sos_get_header_address(mod_header), isr);
}

int8_t ker_sys_register_isr(sos_interrupt_t int_id, uint8_t isr_fid) {
	sos_pid_t mod_id = ker_get_current_pid();
	return ker_register_isr(mod_id, int_id, isr_fid);
}

int8_t ker_register_isr(sos_pid_t mod_id, sos_interrupt_t int_id, uint8_t isr_fid) {
	HAS_CRITICAL_SECTION;

	if ( (int_id >= MAX_USER_INTERRUPTS) ||
		 (mod_id == NULL_PID) ||
		 (pid[int_id] != NULL_PID) ) {
		return -EINVAL;
	}

	ENTER_CRITICAL_SECTION();

	if (ker_fntable_subscribe(PROC_INTERRUPT_PID, mod_id, isr_fid, int_id) < 0) {
		LEAVE_CRITICAL_SECTION();
		return -EINVAL;
	}

	pid[int_id] = mod_id;

	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

int8_t ker_sys_deregister_isr(sos_interrupt_t int_id) {
	sos_pid_t mod_id = ker_get_current_pid();
	return ker_deregister_isr(mod_id, int_id);
}

int8_t ker_deregister_isr(sos_pid_t mod_id, sos_interrupt_t int_id) {
	HAS_CRITICAL_SECTION;
	
	if ( (int_id >= MAX_USER_INTERRUPTS) ||
		 (mod_id == NULL_PID) ||
		 (pid[int_id] != mod_id) ) {
		return -EINVAL;
	}

	ENTER_CRITICAL_SECTION();
	
	pid[int_id] = NULL_PID;
	
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

// User Int vector
interrupt (PORT2_VECTOR) port2_isr() {
	uint8_t p2ifg = P2IFG;
	if (p2ifg & BV(7)) {
		P2IFG &= ~BV(7);
		if (pid[PORT2_INTERRUPT] != NULL_PID) {
			SOS_CALL(isr[PORT2_INTERRUPT], isr_func_t);
		}
	} else {
		P2IFG = 0;
	}
}

