/**
 * @file sos_tool.c
 * @brief  sos_tool interface use to install modules and inject messages into the network
 * @author Simon Han (simonhan@ee.ucla.edu)
 * @author Nicholas Kottenstette (nkottens@nd.edu)
 */
#include <sos.h>
#include <unistd.h>
#include <management/loader/loader.h>
#include <getopt.h>

extern int sos_argc;
extern char** sos_argv;
extern char* sos_emu_short_opts;
mod_header_ptr loader_get_header();
static void tool_insmod(char *optarg);
static void tool_rmmod(char *optarg);
static void tool_lsmod(void);
static void tool_help(void);
static void tool_ldmod(char *optarg);
static void tool_lddata(char *optarg);
static void tool_send_message(char *optarg);

enum {INSMOD_OPT=1,RMMOD_OPT,LSMOD_OPT,SEND_MESSAGE_OPT,
      HELP_OPT,LDDATA_OPT,RMDATA_OPT,LSDATA_OPT,LDMOD_OPT};
static struct option long_options[] = {
    {"insmod", required_argument, NULL, INSMOD_OPT},
    {"rmmod", required_argument, NULL, RMMOD_OPT},
    {"lsmod",no_argument, NULL, LSMOD_OPT},
    {"send_message",required_argument, NULL, SEND_MESSAGE_OPT},
    {"help",no_argument, NULL, HELP_OPT},
    {"lddata", required_argument, NULL, LDDATA_OPT},
    {"rmdata", required_argument, NULL, RMDATA_OPT},
    {"lsdata", no_argument, NULL, LSDATA_OPT},
    {"ldmod",required_argument, NULL, LDMOD_OPT},
    {NULL, 0, NULL, 0},
};

#define long_opt_is(id) (!strcmp(long_options[option_index].name,(id)))
#define tool_error(arg...){\
    fprintf(stderr,arg);   \
    tool_help();           \
    exit(1);               \
}

static void tool_insmod(char *optarg)
{
    int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_INSMOD, strlen(optarg), optarg, 0);
    if(ret < 0){
        tool_error("tool_insmod(%s) failed\n",optarg);
    }
}

static void tool_ldmod(char *optarg)
{
    int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_LDMOD, strlen(optarg), optarg, 0);
    if(ret < 0){
        tool_error("tool_ldmod(%s) failed\n",optarg);
    }
}

static void tool_lddata(char *optarg)
{
    int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_LDDATA, strlen(optarg), optarg, 0);
    if(ret < 0){
        tool_error("tool_lddata(%s) failed\n",optarg);
    }
}

static void tool_rmmod(char *optarg)
{
    int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_RMMOD, strlen(optarg), optarg, 0);
    if(ret < 0){
        tool_error("tool_rmmod(%s) failed\n",optarg);
    }
}

static void tool_rmdata(char *optarg)
{
	int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_RMDATA, strlen(optarg), optarg, 0);
	if( ret < 0 ) {
        tool_error("tool_rmdata(%s) failed\n",optarg);
	}
}

static void tool_send_message(char *optarg)
{
    printf("tool_send_message(%s)\n",optarg);
    process_user_input(optarg);
}

static void tool_lsmod(void)
{
    int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_LSMOD, 0, NULL, 0);
    if(ret < 0){
        tool_error("tool_lsmod failed\n");
    }
}

static void tool_lsdata(void)
{
	int ret;
    ret = post_long(KER_DFT_LOADER_PID, NULL_PID, MSG_LOADER_LSDATA, 0, NULL, 0);
    if(ret < 0){
        tool_error("tool_lsdata failed\n");
    }
}

static void tool_help(void)
{
    printf("sos_tool [%s] --insmod=<module.sos> --rmmod=PID --send_message=<message> --lsmod --help\n",
           sos_emu_short_opts);
    printf("[%s] are the sos_emu_short_opts.  sos_tool -h for more info.\n",sos_emu_short_opts);
    printf("sos_tool long options:\n");
    printf("\tinsmod=<module.sos>: Install a module with the filename <module.sos>\n");
    printf("\tsend_message=<message>: Send a network message <message>\n");
    printf("\t  <message> format: did sid daddr saddr type argc 0xbe 0xef ...\n");
    printf("\trmmod=<code_id>: Remove a module image with <code_id>, note code_id = 0 removes all modules on network\n");
    printf("\tlsmod: List all the module PID's on the network\n.");
}

void sos_start(void)
{
    int ch;
    ker_register_module(loader_get_header());
    
    optind = 0;
    while(1){
        int option_index = 0;
        ch = getopt_long_only(sos_argc, sos_argv, sos_emu_short_opts,
                              long_options, &option_index);
        if (ch == -1)
            break;
        switch(ch){
        case INSMOD_OPT: 
            tool_insmod(optarg);
            return;
        case RMMOD_OPT:
            tool_rmmod(optarg);
            return;
        case LSMOD_OPT:
            tool_lsmod();
            return;
        case LDMOD_OPT:
            tool_ldmod(optarg);
            return;
        case SEND_MESSAGE_OPT:
            tool_send_message(optarg);
            sleep(1);
            exit(0);
        case HELP_OPT:
            tool_help();
            exit(0);
        case LDDATA_OPT:
            tool_lddata(optarg);
            return;
        case RMDATA_OPT:
            //printf("get rmdata, option_indx = %d\n", option_index);
			tool_rmdata(optarg);
            return;
        case LSDATA_OPT:
            //printf("get lsdata, option_indx = %d\n", option_index);
			tool_lsdata();
            return;
        }
    }
    tool_help();
    exit(1);
}

