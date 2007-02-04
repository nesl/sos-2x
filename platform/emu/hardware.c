/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief    hardware related routines
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  0.1
 *
 */

#include <hardware.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <timer_conf.h>
#include <getopt.h>
#include <server.h>
#include <kertable.h>
#include <kertable_proc.h>


#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)
void* ker_jumptable[128] =
SOS_KER_TABLE( CONCAT_TABLES(PROC_KER_TABLE , PLAT_KER_TABLE) );
#elif defined(PROC_KER_TABLE)
void* ker_jumptable[128] =
SOS_KER_TABLE(PROC_KER_TABLE);
#elif defined(PLAT_KER_TABLE)
void* ker_jumptable[128] =
SOS_KER_TABLE(PLAT_KER_TABLE);
#else
void* ker_jumptable[128] =
SOS_KER_TABLE(NULL);
#endif

char *sossrv_addr = "127.0.0.1\0";
int sossrv_port = 7915;

char *sos_named_fifo = NULL;
int sos_argc;
char** sos_argv;
char *sos_emu_short_opts = "hn:s:p:a:f:b:";
//! global variable for critical section
uint32_t int_enabled = 0;

int uart_tcp_port = -1;
//int server_tcp_port = -1;

//! simulation on hardware sleep

void hardware_init(void)
{
	systime_init();
    timer_hardware_init(DEFAULT_INTERVAL, DEFAULT_SCALE);
	radio_init();
	ADCControl_init();
	//flash_init();
	exflash_init();
	server_init();
}

static bool in_exit = false;

void hardware_exit( int code )
{
	//void *v;
	if( in_exit == false ) {
		in_exit = true;
		//printf("Terminating Raio...\n");
		radio_final();
		//printf("Terminating Timer...\n");
		timer_hardware_terminate();
		//printf("Terminating User thread\n");
		exit( code );
	}
}

void hw_get_interrupt_lock( void )
{
	int_enabled = 0;
}

void hw_release_interrupt_lock( void )
{
	int_enabled = 1;
}


void cli_pc(void){
	int_enabled = 0;
}

void sei_pc(void){
	int_enabled = 1;
}


void hardware_sleep()
{
	interrupt_loop();
}

void hardware_wakeup()
{
}

void sig_handler(int sig)
{
    switch(sig){
        case SIGTERM: case SIGINT:
            //DEBUG("caught SIGTERM or SIGINT\n");
			hardware_exit(1);
            exit(1);
            break;
        case SIGPIPE:
            //DEBUG("caught SIGPIPE\n");
			hardware_exit(1);
            exit(1);
            break;
        default:
            break;
    }
}

void process_user_input(char *user_input)
{
    int i_did = 0;
    int i_sid = 0;
    int i_daddr = 0;
    int i_saddr = 0;
    int i_type = 0;
    int i_arg = 0;

    uint8_t data[255];
    uint8_t *msg_data;
    int i;
    int count;
    int len;
    int i_hex;
    int tmp;

    Message m;



	len = strlen(user_input);

	tmp = sscanf(user_input, "%d %d %d %d %d %d %n",
			&i_did, &i_sid, &i_daddr, &i_saddr, &i_type, &i_arg, &count);


	// Check for a negative value and quit if it is found
	if (i_did < 0) {
		printf("gateway terminating...\n");
		exit(1);
	}

	// Allow an easy channel to insert modules
	if (i_did > 999) {

		// Find end of string and mark with null byte
		sscanf(user_input, "%d %n", &i_did, &count);
		i_did -= 1000;
		tmp = strlen(user_input+count);
		user_input[count+tmp-1] = '\0';
		{
			uint8_t buf_size = (uint8_t)(strlen(&(user_input[count])) + 1);
			uint8_t *cmd_buf = ker_malloc(buf_size, USER_PID);
			if(cmd_buf == NULL){
				printf("Not enough memory for processing command\n");
				printf("You can try again if the command is not long\n");
			} else {
				strcpy((char*)cmd_buf, &(user_input[count]));
				post_long((uint8_t)(i_did),
						USER_PID,
						MSG_FROM_USER,
						buf_size,
						cmd_buf,
						SOS_MSG_RELEASE);
			}
		}
	}

	// Assume user wants to send a packet
	// Must be a propely formatted packet with 6 header fields
	if (tmp < 6) {
		return;
	}

	// Parse the data payload
	i = 0;
	while (count < len) {
		sscanf(user_input+count, "%x %n", &i_hex, &tmp);
		data[i] = (uint8_t) i_hex;
		count += tmp;
		i++;
	}

	// Make sure that the payload matches the length specified in the
	// ..header
	if (i != i_arg) {
		return;
	}

	// Constrect and send packt
	msg_data = (uint8_t *) ker_malloc(len, USER_PID);
	if(msg_data) {
		memcpy(msg_data, data, i);
		m.did = (uint8_t) i_did;
		m.sid = (uint8_t) i_sid;
		m.daddr = (uint16_t) i_daddr;
		m.saddr = (uint16_t) i_saddr;
		m.type = (uint8_t) i_type;
		m.len = (uint8_t) i_arg;
		m.data = msg_data;
		m.flag = SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO;
		post(&m);
	}
}

static void user_input_interrupt( int fd )
{
	uint8_t user_input[256];
	fgets((char*)user_input, 255, stdin);
	process_user_input((char*) user_input);
	printf("SOS Command Line: (Usage: <module id + 1000> <command string>)\n");
}

void open_user_thread(void)
{
	if(sos_named_fifo == NULL) {
		printf("setting user input\n");
		interrupt_add_read_fd(0, user_input_interrupt);
	} else {
		/*
		 * References: Support for named pipes derived from redirect.c,
		 * contributed by Nicholas Kottenstette.
		 * see:
		 * http://nesl.ee.ucla.edu/pipermail/sos-user/attachments/20050422/21fa90ba/redirect.bin
		 * for original source code.
		 * The source code and LICENSE associated are also stored in
		 * $SOSROOT/contrib/named_fifo
		 */
#if 0
		FILE *fp;
		int tc;
		int index=0;
		if( !(fp = fopen(sos_named_fifo,"r")) ){
			fprintf(stderr,"Error: unable to open %s\n", sos_named_fifo);
			exit(1);
		}

		while(1) {
			tc = fgetc(fp);
			if (index < 255 && tc != EOF) {
				user_input[index] = (char)tc;
				index++;
			} else {
				user_input[index] = 0;
				process_user_input((char*)user_input);
				index = 0;
				fclose(fp);
				if( !(fp = fopen(sos_named_fifo,"r")) ){
					fprintf(stderr,"Error: unable to open %s\n", sos_named_fifo);
					exit(1);
				}
			}
		}
#endif
	}
}

static void print_help()
{
	printf("SOS PC Tool\n");
	printf("<tool name> [-n <node address>] [-a <sossrv ip address>] [-p <sossrv port>] [-d] [-h]\n");
	//printf(" -o <uart device>         Use this instead of %s\n", uart_dev);
	printf(" -n <node address>        Set node address. Default = %d\n", node_address);
	printf(" -a <sossrv ip address>   Set Sossrv IP address. Default = %s\n", sossrv_addr);
	printf(" -p <sossrv port>         Set Sossrv port number. Default = %d\n", sossrv_port);
	printf(" -s <port>                Enable UART at TCP <port>.  Default = disabled\n");
	printf(" -b <port>                Enable Server as TCP <port>. Default = disabled\n");
	printf(" -f <named fifo>          read input from named fifo instead of stdin\n");
	printf(" -h                       Print this help\n");
}

int main(int argc, char **argv)
{
	int ch, option_index = 0;

	sos_argc = argc;
	sos_argv = argv;

	if(signal(SIGTERM, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGTERM failed\n");
		exit(1);
	}

	if(signal(SIGINT, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGINT failed\n");
		exit(1);
	}
	node_address = 0x8000;
	//! parse the parameters
	if( getenv("SOSROOT") == NULL ) {
		fprintf(stderr, "Environment Variable SOSROOT is not set!\n");
		fprintf(stderr, "Please add SOSROOT to your shell environment\n");
		fprintf(stderr, "SOSROOT should point to the top level directory of SOS\n");
		exit(1);
	}

	while((ch = getopt_long(argc, argv, sos_emu_short_opts,NULL,&option_index)) != -1){
		switch(ch){
			case '?':
				break;
			case 'h':
				print_help();
				exit(1);
				break;

			case 'n': node_address = (uint16_t)atoi(optarg); break;
			case 'p': sossrv_port = (int)atoi(optarg); break;
			case 'a': sossrv_addr = optarg; break;
			case 'f': sos_named_fifo = optarg; break;
			case 's': uart_tcp_port = (int)atoi(optarg); break;
			case 'b': server_tcp_port = atoi(optarg); break;
		}
        if(ch=='?')
            break;
	}
	printf("Gateway Address = 0x%04X\n", node_address);
	//printf("Connecting Gateway to Mote using %s...\n\n", uart_dev);
	//! spawn a thread to get user input
	interrupt_init();
	open_user_thread();
	sos_main(SOS_BOOT_NORMAL);
	return 0;
}

void msg_header_out(uint8_t *prefix, Message *m)
{
    DEBUG("%s msg: daddr(%d)\tdid(%d)\ttype(%d)\tsaddr(%d)\tsid(%d)\targ(%d)\n", prefix, m->daddr, m->did, m->type, m->saddr, m->sid, m->len);
}

