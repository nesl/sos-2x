
#include <hardware.h>
#include <exflash.h>

typedef struct exflash_page {
	uint8_t page[264];
} exflash_page_t;

static exflash_page_t exflash[2048];

static int8_t exflash_handler(void *state, Message *e);
static mod_header_t mod_header SOS_MODULE_HEADER ={
mod_id : EXFLASH_PID,
state_size : 0,
num_sub_func : 0,
num_prov_func : 0,
module_handler: exflash_handler,
};  
static sos_module_t exflash_module;

int8_t ker_exflash_read(sos_pid_t pid,
		exflashpage_t page, exflashoffset_t offset,
		void *reqdata, exflashoffset_t n)
{
	uint8_t *d = (uint8_t *) reqdata;

	memcpy(d, &(exflash[page].page[offset]), n);
	post_short(pid, EXFLASH_PID, MSG_EXFLASH_READDONE, true, 0, 0);
	return SOS_OK;
}

int8_t ker_exflash_write(sos_pid_t pid,                       
		exflashpage_t page, exflashoffset_t offset,           
		void *reqdata, exflashoffset_t n)                     
{                                                             
	uint8_t *d = (uint8_t *) reqdata;
	memcpy(&(exflash[page].page[offset]), d, n);
	post_short(pid, EXFLASH_PID, MSG_EXFLASH_WRITEDONE, true, 0, 0);
	return SOS_OK;
}

int8_t ker_exflash_flushAll(sos_pid_t pid)
{
	post_short(pid, EXFLASH_PID, MSG_EXFLASH_FLUSHDONE, true, 0, 0);
	return SOS_OK;
}

int8_t ker_exflash_flush(sos_pid_t pid, exflashpage_t page)
{
	post_short(pid, EXFLASH_PID, MSG_EXFLASH_FLUSHDONE, true, 0, 0);
	return SOS_OK;
}

static int8_t exflash_handler(void *state, Message *e)
{
	return -EINVAL;
}

int8_t exflash_init()
{
	sched_register_kernel_module(&exflash_module, sos_get_header_address(mod_header), NULL);
	return SOS_OK;
}
