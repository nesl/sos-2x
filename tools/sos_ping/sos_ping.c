/**
 * @file sos_ping.c
 * @brief  sos_ping is for pinging the node.
 * @author Simon Han (simonhan@ee.ucla.edu)
 */
#include <sos.h>
#include <unistd.h>
#include <loader/loader.h>
#include <getopt.h>
#include <management/ping/ping.h>

extern int sos_argc;
extern char** sos_argv;
extern char* sos_emu_short_opts;

mod_header_ptr neighbor_get_header();
mod_header_ptr aodv_get_header();
mod_header_ptr ping_get_header();



static struct option long_options[] = {
	{"node", required_argument, NULL, 1},
	{"count", required_argument, NULL, 2},
	{NULL, 0, NULL, 0},
};

static void tool_help(void)
{
	printf("sos_ping --node=<id> --count=<count>\n");
}

static ping_req_t req;

void sos_start(void)
{
    int ch;
	ker_register_module(neighbor_get_header());
	ker_register_module(aodv_get_header());
	ker_register_module(ping_get_header());

	req.count = 0;
	req.addr  = BCAST_ADDRESS;

    optind = 0;
    while(1){
        int option_index = 0;
        ch = getopt_long_only(sos_argc, sos_argv, sos_emu_short_opts,
                              long_options, &option_index);
        if (ch == -1)
            break;
        switch(ch){
		case 1:
			req.addr = (uint16_t) atoi( optarg );
			break;
		case 2:
			req.count = (uint16_t) atoi( optarg );
			break;
        }
    }
	if( req.addr == BCAST_ADDRESS ) {
		printf("No address is defined\n");
		tool_help();
		exit(1);
	}
	
	post_long(PING_PID, PING_PID, MSG_SEND_PING, sizeof(ping_req_t), &req, 0);
}

