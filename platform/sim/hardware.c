/**
 * @brief    hardware related routines
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  0.1
 * 9/24/2005 mods made by Nicholas Kottenstette (nkottens@nd.edu)
 *      added additional long options to set global kernel values
 *      for each "simulated-mote"
 */

#include <hardware.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sos_info.h>
#include <sos_timer.h>
#include <kertable.h>
#include <kertable_proc.h>
#include <uart_hal.h>
#include <server.h>

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

/**
 * global variables
 */

char *topofile = "../../tools/admin/topo.def\0";
uint16_t radio_pkt_success_rate = 100;  //!< 100%
int uart_tcp_port = -1;
//int server_tcp_port = -1;
int radio_xmit_power = 0xff;
uint32_t int_enabled = 0;
int debug_socket = -1;
struct sockaddr_in debug_addr;


void hardware_init(void){
	systime_init();
	radio_init();
	led_init();
	timer_hardware_init(DEFAULT_INTERVAL, DEFAULT_SCALE);

	uart_system_init();
#ifndef NO_SOS_UART
	  //! Initalize uart comm channel
	sos_uart_init();
#endif

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
		//printf("Terminating Radio...\n");
		radio_final();
		//printf("Terminating UART...\n");
		uart_hardware_terminate();
		//printf("Terminating Timer...\n");
		timer_hardware_terminate();
		exit( code );
	}
}

void hw_get_interrupt_lock( void )
{
}

void hw_release_interrupt_lock( void )
{
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
        break;
    default:
        break;
    }
}

static void userThread(int fd)
{
    char user_input[255];
    int i_did;
    int i_sid;
    int i_daddr;
    int i_saddr;
    int i_type;
    int i_arg;

    uint8_t data[255];
    uint8_t *msg_data;
    int i;
    int count;
    int len;
    int i_hex;
    int tmp;

    Message m;

    {

        printf("SOS Command Line: (Usage: <module id + 1000> <command string>)\n");
        i_did = 0;
        i_sid = 0;
        i_daddr = 0;
        i_saddr = 0;
        i_type = 0;
        i_arg = 0;

        // Read packet from input
        fgets(user_input, 255, stdin);
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
}

static void print_help()
{
	printf("SOS Sim Tool\n");
    printf("------------\n");
    printf("\n");
    printf("Basic Usage:\n");
    printf("\n");
	printf("<sim file name> -n <node address> [-f <topology file>] [-p <packet success rate>] [-u] [-h]\n");
	printf(" -n <node address>              Set node address (required)\n");
	printf(" -f <topology file>             Set topology file. Default = %s\n", topofile);
	printf(" -p <packet success rate>       Set packet success rate. Default = %d%%\n", radio_pkt_success_rate);
	printf(" -u                             Enable user input.  Default = false\n");
	printf(" -s <port>                      Enable UART at TCP <port>.  Default = disabled\n");
	printf(" -b <port>                      Enable Server as TCP <port>. Default = disabled\n");
	printf(" -h                             Print this help\n");
    printf("\n");
    printf("Long Options:\n");
    printf("\n");
    printf(" --node_address <node address>  Set node address\n");
    printf(" --file <topology file>         Set topology file\n");
    printf(" --packet_loss <pkt succ rate>  Set packet success rate\n");
    printf(" --user_input                   Enable user input\n");
    printf(" --socket <port>                Enaple UART on TCP port\n");
    printf(" --help                         Print this help\n");
    printf(" --node_group_id <id>           Set node group ID\n");
    printf(" --hw_type <hw type>            Set node hardware type\n");
    printf(" --radio_xmit_power <TX power>  Set node TX power\n");
    printf(" --node_loc.unit <units>        Set node location units\n");
    printf(" --node_loc.x <x location>      Set node location x\n");
    printf(" --node_loc.y <y location>      Set node location y\n");
    printf(" --node_loc.z <z location>      Set node location z\n");
    printf(" --gps_loc.x.dir <gps x dir>    Set node gps x direction\n");
    printf(" --gps_loc.x.deg <gps x deg>    Set node gps x degrees\n");
    printf(" --gps_loc.x.min <gps x min>    Set node gps x minutes\n");
    printf(" --gps_loc.x.sec <gps x sec>    Set node gps x seconds\n");
    printf(" --gps_loc.y.dir <gps y dir>    Set node gps y direction\n");
    printf(" --gps_loc.y.deg <gps y deg>    Set node gps y degrees\n");
    printf(" --gps_loc.y.min <gps y min>    Set node gps y minutes\n");
    printf(" --gps_loc.y.sec <gps y sec>    Set node gps y seconds\n");
    printf(" --gps_loc.z <gps z location>   Set node gps z location\n");
    printf(" --gps_loc.unit <gps unit>      Set node gps units\n");
}

static void debug_socket_init(void)
{
#define MAXLENHOSTNAME 256
    struct hostent *hostptr;
    char theHost[256];
    unsigned long hostaddress;

    debug_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(debug_socket < 0){
        perror("unable to create debug_socket\n");
        exit(1);
    }
    /* find out who I am */
    if ((gethostname(theHost, MAXLENHOSTNAME))<0) {
        perror("could not get hostname\n");
        exit(1);
    }
    if ((hostptr = gethostbyname (theHost)) == NULL) {
        perror("could not get host by name, use 127.0.0.1\n");
		if(( hostptr = gethostbyname ("127.0.0.1") ) == NULL ) {
			perror ("Cannot get host 127.0.0.1");
			exit (1);
		}
        //exit (1);
    }
    hostaddress = *((unsigned long *) hostptr->h_addr);

    debug_addr.sin_family = AF_INET;
    debug_addr.sin_port = htons( SIM_DEBUG_PORT );
    debug_addr.sin_addr.s_addr = hostaddress;
}

static struct option long_options[] = {
    {"node_address", 1, 0, 'n'},
    {"file", 1, 0, 'f'},
    {"packet_loss", 1, 0, 'p'},
    {"user_input", 0, 0, 'u'},
    {"socket", 1, 0, 's'},
    {"help", 0, 0, 'h'},
    {"node_group_id", 1, 0, 0},
    {"hw_type", 1, 0, 0},
    {"radio_xmit_power", 1, 0, 0},
    {"node_loc.unit", 1, 0, 0},
    {"node_loc.x", 1, 0, 0},
    {"node_loc.y", 1, 0, 0},
    {"node_loc.z", 1, 0, 0},
    {"gps_loc.x.dir", 1, 0, 0},
    {"gps_loc.x.deg", 1, 0, 0},
    {"gps_loc.x.min", 1, 0, 0},
    {"gps_loc.x.sec", 1, 0, 0},
    {"gps_loc.y.dir", 1, 0, 0},
    {"gps_loc.y.deg", 1, 0, 0},
    {"gps_loc.y.min", 1, 0, 0},
    {"gps_loc.y.sec", 1, 0, 0},
    {"gps_loc.unit", 1, 0, 0},
    {"gps_loc.z", 1, 0, 0},
    {0, 0, 0, 0},
};


int main(int argc, char **argv)
{
	int ch;
	if(signal(SIGTERM, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGTERM failed\n");
		exit(1);
	}

	if(signal(SIGINT, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGINT failed\n");
		exit(1);
	}

	if( getenv("SOSROOT") == NULL ) {
		fprintf(stderr, "Environment Variable SOSROOT is not set!\n");
		fprintf(stderr, "Please add SOSROOT to your shell environment\n");
		fprintf(stderr, "SOSROOT should point to the top level directory of SOS\n");
		exit(1);
	}
	interrupt_init();
	//! parse the parameters
	while(1){
            int option_index = 0;

            ch = getopt_long(argc, argv, "huf:p:s:n:b:",
                             long_options, &option_index);
            if (ch == -1)
                break;
#define long_opt_is(id) (!strcmp(long_options[option_index].name,(id)))
            switch(ch){
            case 0:
                if(long_opt_is("node_group_id")){
                    node_group_id = atoi(optarg);
                    printf("node_group_id = %d\n",node_group_id);
                }else if(long_opt_is("hw_type")){
                    hw_type = atoi(optarg);
                    printf("hw_type = %d\n",hw_type);
                }else if(long_opt_is("node_address")){
                    node_address = atoi(optarg);
                    printf("node_address = %d\n",node_address);
                }else if(long_opt_is("radio_xmit_power")){
                    radio_xmit_power = atoi(optarg);//XXX
                    printf("radio_xmit_power = %d\n",radio_xmit_power);
                }else if(long_opt_is("node_loc.unit")){
                    node_loc.unit = atoi(optarg);
                    printf("node_loc.unit = %d\n",node_loc.unit);
                }else if(long_opt_is("node_loc.x")){
                    node_loc.x = atoi(optarg);
                    printf("node_loc.x = %d\n",node_loc.x);
                }else if(long_opt_is("node_loc.y")){
                    node_loc.y = atoi(optarg);
                    printf("node_loc.y = %d\n",node_loc.y);
                }else if(long_opt_is("node_loc.z")){
                    node_loc.z = atoi(optarg);
                    printf("node_loc.z = %d\n",node_loc.z);
                }else if(long_opt_is("gps_loc.x.dir")){
                    gps_loc.x.dir = atoi(optarg);
                    printf("gps_loc.x.dir = %d\n",gps_loc.x.dir);
                }else if(long_opt_is("gps_loc.x.deg")){
                    gps_loc.x.deg = atoi(optarg);
                    printf("gps_loc.x.deg = %d\n",gps_loc.x.deg);
                }else if(long_opt_is("gps_loc.x.min")){
                    gps_loc.x.min = atoi(optarg);
                    printf("gps_loc.x.min = %d\n",gps_loc.x.min);
                }else if(long_opt_is("gps_loc.x.sec")){
                    gps_loc.x.sec = atoi(optarg);
                    printf("gps_loc.x.sec = %d\n",gps_loc.x.sec);
                }else if(long_opt_is("gps_loc.y.dir")){
                    gps_loc.y.dir = atoi(optarg);
                    printf("gps_loc.y.dir = %d\n",gps_loc.y.dir);
                }else if(long_opt_is("gps_loc.y.deg")){
                    gps_loc.y.deg = atoi(optarg);
                    printf("gps_loc.y.deg = %d\n",gps_loc.y.deg);
                }else if(long_opt_is("gps_loc.y.min")){
                    gps_loc.y.min = atoi(optarg);
                    printf("gps_loc.y.min = %d\n",gps_loc.y.min);
                }else if(long_opt_is("gps_loc.y.sec")){
                    gps_loc.y.sec = atoi(optarg);
                    printf("gps_loc.y.sec = %d\n",gps_loc.y.sec);
                }else if(long_opt_is("gps_loc.unit")){
                    gps_loc.unit = atoi(optarg);
                    printf("gps_loc.unit = %d\n",gps_loc.unit);
                }else if(long_opt_is("gps_loc.z")){
                    gps_loc.z = atoi(optarg);
                    printf("gps_loc.z = %d\n",gps_loc.z);
                }
                break;
            case '?': case 'h':
                print_help();
                exit(1);
                break;
            case 'f':
                topofile = optarg;
                break;
            case 'p':
                radio_pkt_success_rate = atoi(optarg);
                break;
            case 'u':
				interrupt_add_read_fd(0, userThread);
                break;
            case 's':
                uart_tcp_port = atoi(optarg);
                break;
            case 'n':
                node_address = atoi(optarg);
                break;
            case 'b':
            	server_tcp_port = atoi(optarg);
            	break;
            }
	}
#if 0
	if(node_address == BCAST_ADDRESS) {
		fprintf(stderr, "-n <node address> is required\n");
		fprintf(stderr, "<node address> cannot be %d\n", BCAST_ADDRESS);
		exit(1);
	}
#endif
    debug_socket_init();
	sim_radio_init();

	sos_main(SOS_BOOT_NORMAL);
	return 0;
}

void msg_header_out(uint8_t *prefix, Message *m)
{
	DEBUG("%s msg: daddr(%d)\tdid(%d)\ttype(%d)\tsaddr(%d)\tsid(%d)\tlen(%d) (%x)\n", prefix, m->daddr, m->did, m->type, m->saddr, m->sid, m->len, (unsigned int)m->data);
}

