/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
#include <module.h>
#include <sos.h>
#include <sos_sched.h>
#include <sos_module_fetcher.h>
#include <led_dbg.h>
#include "loader.h"
#include <sos_cam.h>
#include <ctype.h>
#ifdef MINIELF_LOADER
#include <melfloader.h>
#endif


/**
 * Configurable parameters
 */
//#define SOS_DEBUG_DFT_LOADER
#ifndef SOS_DEBUG_DFT_LOADER
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_PID
#define DEBUG_PID(...)
#define print_mod_header(x)
#else
void print_mod_header(mod_header_t* mod_hdr);
#endif


enum {
  PERIOD_INTERVAL     =   5L * 1024L,    // 1 second
  MAX_IMAGE_SIZE      =   64L * 256L,
  BUF_SIZE            =   255L,
  RMALL_CODE_ID       =     0L,
};

static int8_t loader_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER =
  {
	.mod_id             = KER_DFT_LOADER_PID,
	.state_size         = 0,
	.num_timers         = 0,
	.num_sub_func       = 0,
	.num_prov_func      = 0,
	.module_handler     = loader_handler,
  };

/**
 * Timer TIDs
 */
enum {
  TRAN_TID  = 1,
};

/**
 * protocol state
 * Wait for 2 pieces of information from network
 * 1. module version vector
 * 2. module code_id vertor
 *
 */
enum {
  WAIT_LSMOD      = 0,
  WAIT_MOD_VECTOR = 1,
  WAIT_UNTIL_SYNC = 2,
};

/**
 * State
 */
typedef struct {
  sos_timer_t tran_timer;
  sos_timer_t data_timer;
  msg_version_data_t *version_data;
  uint8_t state;
  msg_ls_reply_t *reply;
  char* image_filename;
  sos_code_id_t    rmmod_code_id;
  uint8_t type_requested;   
  uint8_t rmdata_pid;
  uint8_t rmdata_handle;
  // message type requested (can be MSG_LOADER_INSMOD, MSG_LOADER_LSMOD, MSG_LOADER_RMMOD
} dft_loader_t;


/*
 * State data
 */
static dft_loader_t  st;
static unsigned char image_buf[MAX_IMAGE_SIZE];    //!< buffer to store the image
static int addr_end;                   //!< end of image buffer
static sos_pid_t module_id = 0;                          //!< module id of this insmod
static codemem_t flashImage();
static void load_image_from_file( char *filename );
static void load_data_from_file( char *filename );
static void print_lsmod_reply( msg_ls_reply_t *reply, bool debug_flag );
static void check_insmod( msg_ls_reply_t *reply, mod_header_t *mod_hdr );
static void check_rmmod( msg_ls_reply_t *reply, sos_code_id_t code_id );
static void check_lddata( msg_ls_reply_t *reply, uint8_t *databuf );
static void check_rmdata( msg_ls_reply_t *reply, uint8_t pid, uint8_t handle);
static int process_insmod();
static void process_rmmod();
static void process_lddata();
static void process_rmdata();

static void send_adv()
{
  version_t *v;
  DEBUG("send_adv()\n");
	
  v = ker_malloc( sizeof(version_t), KER_DFT_LOADER_PID );
  if( v == NULL ) {
	return;
  }

  *v = version_hton(0);

  post_link(KER_DFT_LOADER_PID, KER_DFT_LOADER_PID,
			MSG_VERSION_ADV, sizeof(version_t), v,
			SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO,
			BCAST_ADDRESS);
  st.state = WAIT_MOD_VECTOR;
}

static void send_data()
{
  msg_version_data_t *d;

  DEBUG("send_data()\n");

  d = (msg_version_data_t *) ker_malloc(
										sizeof( msg_version_data_t ), 
										KER_DFT_LOADER_PID);

  if( d != NULL ) {
	*d = *(st.version_data);

	d->version = version_hton( st.version_data->version );

	post_link(KER_DFT_LOADER_PID, KER_DFT_LOADER_PID, 
			  MSG_VERSION_DATA, 
			  sizeof( msg_version_data_t ),
			  d, 
			  SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO, 
			  BCAST_ADDRESS);
  }
  st.state = WAIT_UNTIL_SYNC;
}

static void send_lsmod()
{
  DEBUG("send_lsmod()\n");
  post_link(KER_DFT_LOADER_PID, KER_DFT_LOADER_PID,
			MSG_LOADER_LS_ON_NODE, 0, NULL,
			SOS_MSG_ALL_LINK_IO,
			BCAST_ADDRESS);
  st.state = WAIT_LSMOD;
}

static int8_t handle_timeout( Message *msg )
{
  MsgParam* params = ( MsgParam * )( msg->data );

  switch( params->byte ) {
  case TRAN_TID:
	{
	  if( st.state == WAIT_LSMOD ) {
		send_lsmod();
		return SOS_OK;
	  }
	  if( st.state == WAIT_MOD_VECTOR ) {
		send_adv();
		return SOS_OK;
	  }
	  if( st.state == WAIT_UNTIL_SYNC ) {
		send_data();
		return SOS_OK;
	  }
	  break;
	}
  }
  return SOS_OK;
}

static int8_t handle_version_adv( Message *msg )
{
  msg_version_adv_t *pkt = (msg_version_adv_t *) msg->data;

  DEBUG("handle_version_adv()\n");
  pkt->version = version_ntoh(pkt->version);	

  if( st.state != WAIT_UNTIL_SYNC ) {
	return SOS_OK;
  }

  if( pkt->version == st.version_data->version ) {
	printf("loading complete\n");
	hardware_exit(0);
  }
  return SOS_OK;
}

static int8_t handle_version_data( Message *msg )
{
  msg_version_data_t *pkt = (msg_version_data_t *) msg->data;

  DEBUG("handle_version_data()\n");
  pkt->version = version_ntoh(pkt->version);

  if( st.state != WAIT_MOD_VECTOR) {
	return SOS_OK;
  }
  st.version_data = (msg_version_data_t*) 
	ker_msg_take_data(KER_DFT_LOADER_PID, msg);
	
  if( st.type_requested == MSG_LOADER_INSMOD || 
	  st.type_requested == MSG_LOADER_LDMOD) {	
	process_insmod();
  }

  if( st.type_requested == MSG_LOADER_LDDATA ) {
	process_lddata();
  }

  if( st.type_requested == MSG_LOADER_RMMOD ) {
	process_rmmod();
  }

  if( st.type_requested == MSG_LOADER_RMDATA ) {
	process_rmdata();
  }
  send_data();
  return SOS_OK;
}

static void process_lddata()
{
  codemem_t cm;
  fetcher_cam_t *cam;
  int i;
  uint8_t idx = 0;
  sos_cam_t key;

  cam = ker_malloc(sizeof(fetcher_cam_t), KER_DFT_LOADER_PID);
  if( cam == NULL ) {
	printf("ERROR: no memory for CAM\n");
	hardware_exit(1);
  }
  for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i++ ) {
	if( st.version_data->pam_size[i] == 0 ) {
	  st.version_data->pam_ver[i] ++;
	  st.version_data->pam_size[i] = (addr_end + (LOADER_SIZE_MULTIPLIER - 1)) / LOADER_SIZE_MULTIPLIER;
	  st.version_data->version ++;
	  idx = i;
	  break;
	}
  }
  cm = flashImage();
  key = ker_cam_key( KER_DFT_LOADER_PID, (idx));

  cam->cm = cm;
  cam->fetchtype = FETCHTYPE_DATA;
  ker_cam_add(key, cam);
  return;
}

static int process_insmod()
{
  mod_header_t *mod_hdr;
  codemem_t cm;
  sos_code_id_t code_id;
  uint8_t idx = 0;
  fetcher_cam_t *cam;
  sos_cam_t key;
  int i;

  cam = ker_malloc(sizeof(fetcher_cam_t), KER_DFT_LOADER_PID);
  if( cam == NULL ) {
	printf("ERROR: no memory for CAM\n");
	hardware_exit(1);
  }
#ifdef MINIELF_LOADER
  mod_hdr = melf_get_mod_header(image_buf);
  DEBUG("Done reading module header from Mini-ELF file\n");
  print_mod_header(mod_hdr);
#else
  mod_hdr = (mod_header_t*) image_buf;
#endif
  module_id = mod_hdr->mod_id;
  code_id = entohs(mod_hdr->code_id);

  for( i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++ ) {
	if( st.version_data->mod_size[i] == 0 ) {
	  st.version_data->mod_ver[i] ++;
	  st.version_data->mod_size[i] = (addr_end + (LOADER_SIZE_MULTIPLIER - 1)) / LOADER_SIZE_MULTIPLIER;
	  st.version_data->version ++;
	  if(st.type_requested == MSG_LOADER_LDMOD) {
		st.version_data->mod_ver[i] &= 0x7f;
	  } else {
		st.version_data->mod_ver[i] |= 0x80;
	  }
	  idx = i;
	  break;
	}
  }

  if( code_id == 0 ) {
	printf("code_id is zero...\n");
	printf("assuming module ID\n");
	mod_hdr->code_id = ehtons( module_id );
	code_id = module_id;
  }
  printf("module id = %d\n", module_id);
  printf("memory size = %d\n", mod_hdr->state_size);
  printf("code id = %d\n", code_id);

  //! print info
  printf("addr_start = %d\n", 0);
  printf("addr_end = %d\n", addr_end);

  cm = flashImage();
  /*
   * store to CAM so that fetcher can find it
   */	
  key = ker_cam_key( KER_DFT_LOADER_PID, (idx + NUM_LOADER_PARAMS_ENTRIES));
	
  cam->cm = cm;
  cam->fetchtype = FETCHTYPE_DATA;
  ker_cam_add(key, cam);

  return 0;
}

static void process_rmmod()
{
  int i;
  if(st.rmmod_code_id == RMALL_CODE_ID ) {
	for( i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++ ) {
	  if( st.reply->code_id[ i ] != 0 ) {
		st.version_data->mod_ver[i] ++;
		st.version_data->mod_size[i] = 0;
	  }
	}
	st.version_data->version ++;
	return;
  } else {
	for( i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++ ) {
	  if( st.reply->code_id[ i ] == st.rmmod_code_id ) {
		st.version_data->mod_ver[i] ++;
		st.version_data->mod_size[i] = 0;
		st.version_data->version ++;
		return;
	  }
	}
  }
}

static void process_rmdata()
{
  int i;
  for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i++ ) {
	if( st.reply->pam_dst_pid[ i ] == st.rmdata_pid &&
		st.reply->pam_dst_subpid[i] == st.rmdata_handle ) {
	  st.version_data->pam_ver[i] ++;
	  st.version_data->pam_size[i] = 0;
	  st.version_data->version ++;
	  return;
	}
  }
	
}


static codemem_t flashImage()
{
  int i;
  codemem_t cm;

  cm = ker_codemem_alloc(addr_end, CODEMEM_TYPE_EXECUTABLE);
  if( cm == CODEMEM_INVALID ) {
	printf("Cannot open codemem for writing!\n");
	printf("Quitting... \n");
	hardware_exit(1);	
  }
  for(i = 0; i < addr_end; 
	  i+= FLASH_PAGE_SIZE) {
	if(addr_end > (i + FLASH_PAGE_SIZE)) {
	  ker_codemem_write(cm, MOD_D_PC_PID, image_buf + i, FLASH_PAGE_SIZE, i);
	} else {
	  ker_codemem_write(cm, MOD_D_PC_PID, image_buf + i, addr_end - i, i);
	}
  }
  ker_codemem_flush(cm, KER_DFT_LOADER_PID);

  return cm;
}

static int8_t handle_lsmod( Message *msg )
{
  ker_timer_start( KER_DFT_LOADER_PID, TRAN_TID, PERIOD_INTERVAL );
  send_lsmod();
  return SOS_OK;
}

static int8_t handle_insmod( Message *msg )
{
  st.image_filename = (char*) msg->data;

  load_image_from_file( st.image_filename );

  return handle_lsmod( msg );
}

static int8_t handle_lddata( Message *msg ) 
{
  st.image_filename = (char *) msg->data;

  printf("load data from %s\n", st.image_filename);
  load_data_from_file( st.image_filename );
  printf("data size = %d\n", addr_end);

  return handle_lsmod( msg );
}

static int8_t handle_rmmod( Message *msg )
{
  int rmmod_id;

  if( sscanf((char *)msg->data, "%d", &rmmod_id) != 1 ) {
	printf("Unable to parse code_id from %s\n", msg->data );
	hardware_exit( 1 );
  }
  st.rmmod_code_id = (sos_code_id_t) rmmod_id;

  return handle_lsmod( msg );
}

static int8_t handle_rmdata( Message *msg ) 
{
  int rm_pid;
  int rm_subpid;

  if( sscanf((char *) msg->data, "%d:%d", &rm_pid, &rm_subpid) != 2 ) {
	printf("Unable to parse pid:handle from %s\n", msg->data );
	printf("sos_tool.exe --rmdata <pid:handle>\n");
	hardware_exit( 1 );
  }

  if( rm_pid > 255 || rm_pid < 0 || rm_subpid > 255 || rm_subpid < 0) {
	printf("Unable to parse pid:handle from %s\n", msg->data );
	printf("sos_tool.exe --rmdata <pid:handle>\n");
	hardware_exit( 1 );
  }
	
  st.rmdata_pid = (uint8_t) rm_pid;
  st.rmdata_handle = (uint8_t) rm_subpid;
  // First to get the listing of data
  return handle_lsmod( msg );
}

static int8_t handle_lsmod_reply( Message *msg )
{
  uint8_t i;

  st.reply = (msg_ls_reply_t *) 
	ker_msg_take_data( KER_DFT_LOADER_PID, msg ); 
  for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	st.reply->code_id[i] = entohs( st.reply->code_id[i] );
  }

#ifdef SOS_DEBUG_DFT_LOADER
  printf("\n *** *** *** Loader PC debug *** *** *** \n");
  print_lsmod_reply( st.reply, true );	
  printf(" *** *** *** *** *** *** *** *** *** *** \n\n");
#endif

  if( st.state != WAIT_LSMOD ) {
	return SOS_OK;
  }


  if( st.type_requested == MSG_LOADER_LSMOD ||
	  st.type_requested == MSG_LOADER_LSDATA) {
	print_lsmod_reply( st.reply, false );
	hardware_exit( 0 );
  }

  if( st.type_requested == MSG_LOADER_INSMOD ) {
#ifdef MINIELF_LOADER
	mod_header_t* mod_hdr;
	mod_hdr = melf_get_mod_header(image_buf);
	DEBUG("Done reading module header from Mini-ELF file\n");
	print_mod_header(mod_hdr);
	check_insmod( st.reply, mod_hdr);
#else
	check_insmod( st.reply, (mod_header_t *) image_buf);
#endif
  }

  if( st.type_requested == MSG_LOADER_LDDATA ) {
	check_lddata( st.reply, (uint8_t *) image_buf);
  }

  if( st.type_requested == MSG_LOADER_RMMOD ) {
	check_rmmod( st.reply, st.rmmod_code_id);	
  }

  if( st.type_requested == MSG_LOADER_RMDATA ) {
	check_rmdata( st.reply, st.rmdata_pid, st.rmdata_handle );
  }
  send_adv();

  return SOS_OK;
}

static void check_lddata( msg_ls_reply_t *reply, uint8_t *databuf )
{
  uint8_t i;
  bool space_found = false;

  for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i++ ) {
	if(databuf[0] == reply->pam_dst_pid[ i ]  &&
	   databuf[1] == reply->pam_dst_subpid[ i ]) {
	  printf("ERROR: destination PID %d with handle %d already exist!\n", databuf[0], databuf[1]);
	  printf("\n");
	  printf("Try to use sos_rmdata.sh <data> to remove data\n");
	  printf("And then try again.\n");
	  hardware_exit( 1 );
	}
  }
  for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i++ ) {
	if( reply->pam_dst_pid[ i ] == NULL_PID ) {
	  space_found = true;
	  break;
	}
  }
  if( space_found == false ) {
	printf("ERROR: No space in loader...\n");
	hardware_exit(1);
  }
}

static void check_insmod( msg_ls_reply_t *reply, mod_header_t *mod_hdr ) 
{
  uint8_t i;
  sos_code_id_t code_id;
  bool space_found = false;

  code_id = entohs(mod_hdr->code_id);
  for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	if( code_id == reply->code_id[ i ] ) {
	  printf("ERROR: the same module with code_id = %d exist in the system\n", code_id );
	  printf("\n");
	  printf("Try to use sos_rmmod <code_id>\n");
	  printf("And then try again.\n");
	  hardware_exit( 1 );
	}
  }
  for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	if( reply->code_id[i] == 0 ) {
	  space_found = true;
	  break;
	}
  }
  if( space_found == false ) {
	printf("ERROR: No space in loader...\n");
	hardware_exit(1);
  }
}

static void check_rmmod( msg_ls_reply_t *reply, sos_code_id_t code_id )
{
  uint8_t i;

  if(st.rmmod_code_id == RMALL_CODE_ID ) {
	for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	  if( 0 != reply->code_id[ i ] ) {
		return;
	  }
	}
	printf("ERROR: nothing to remove\n");
	hardware_exit( 1 );
  } else {
	for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	  if( code_id == reply->code_id[ i ] ) {
		return;
	  }
	}
	printf("ERROR: code_id %d does not exist!\n", code_id);
	hardware_exit( 1 );
  }
}

static void check_rmdata( msg_ls_reply_t *reply, uint8_t pid, uint8_t handle)
{
  int i;
  for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i ++ ) {
	if( reply->pam_dst_pid[i] == pid && 
		reply->pam_dst_subpid[i] == handle ) {
	  return;
	}
  }

  printf("ERROR: pid %d handle %d does not exist!\n", pid, handle);
  hardware_exit( 1 );
}

static void print_lsmod_reply( msg_ls_reply_t *reply, bool debug_flag )
{
  uint8_t i;
  if( st.type_requested == MSG_LOADER_LSMOD || debug_flag == true ) {
	printf("======{ lsmod }=======\n");
	for(i = 0; i < NUM_LOADER_MODULE_ENTRIES; i++) {
	  if( reply->code_id[i] != 0 ) {
		if( i < 10 ) {
		  printf("[ 0%d ] code_id = %d\n", i, reply->code_id[i]);
		} else {
		  printf("[ %d ] code_id = %d\n", i, reply->code_id[i]);
		}	
	  } else {
		if( i < 10 ) {
		  printf("[ 0%d ] no code\n", i);
		} else {
		  printf("[ %d ] no code\n", i);
		}	
	  }
	}	
  } 
  if ( st.type_requested == MSG_LOADER_LSDATA || debug_flag == true ) {
	printf("======{ lsdata }======\n");
	for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i ++ ) {
	  if( reply->pam_dst_pid[i] != NULL_PID ) {
		if( i < 10 ) {
		  printf("[ 0%d ] pid = %d, handle = %d\n", 
				 i, reply->pam_dst_pid[i], reply->pam_dst_subpid[i]);
		} else {
		  printf("[ %d ] pid = %d, handle = %d\n", 
				 i, reply->pam_dst_pid[i], reply->pam_dst_subpid[i]);
		}
	  } else {
		if( i < 10 ) {
		  printf("[ 0%d ] no data\n", i );
		} else {
		  printf("[ %d ] no data\n", i );
		}
	  }
	}
  }
}

static void load_image_from_file( char *filename )
{
  FILE *binFile;
  int cin;
  int i = 0;

  binFile = fopen((char*)filename, "r");

  if(binFile == NULL){
	fprintf(stderr, "%s does not exist\n", filename);
	hardware_exit( 1 );
  }
  addr_end = 0;

  //! initialize image buffer
  for(i = 0; i < MAX_IMAGE_SIZE; i++){
	image_buf[i] = 0xff;
  }

  /** load the image into buffer */
  while((cin = fgetc(binFile)) != EOF){
	image_buf[addr_end] = (unsigned char)cin;
	addr_end++;
  }
  addr_end++;
  fclose(binFile);
}

static void load_data_from_file( char *filename )
{
  FILE *dFile;
  char Buf[4096];
  int i = 0;
  int cin;
  unsigned char din;
	
  dFile = fopen((char*) filename, "r");

  if(dFile == NULL) {
	fprintf(stderr, "%s does not exist\n", filename);
	hardware_exit( 1 );
  }

  addr_end = 0;
  for(i = 0; i < MAX_IMAGE_SIZE; i++){
	image_buf[i] = 0xff;
  }
  i = 0;
  // Parse Hex value...
  // copy to Buf until <Space> or <EOF>
  while(true) {
	cin = fgetc(dFile);
	DEBUG("read %x\n", cin);
	if(isspace(cin) || cin == EOF) {
	  DEBUG("isspace...\n");
	  Buf[i] = 0;
	  DEBUG("Buf = %s\n", Buf);
	  if( i != 0 ) {
		if(sscanf(Buf, "%hhi", &din) != 1) {
		  fprintf(stderr, "Unable to parse %s in %s\n",
				  Buf, filename);
		  fclose(dFile);
		  hardware_exit( 1 );
		}
		DEBUG("parsed %x\n", din);
		image_buf[addr_end] = din;
		addr_end ++;
		i = 0;
	  }
	} else {
	  Buf[i++] = cin;
	}	

	if(cin == EOF) {
	  fclose(dFile);
	  return;
	}
  }
	
}

static int8_t handle_init()
{
  ker_permanent_timer_init((&st.tran_timer), KER_DFT_LOADER_PID,
						   TRAN_TID, TIMER_REPEAT);

  st.version_data = NULL;
  st.image_filename = NULL;

  return SOS_OK;
}

static int8_t loader_handler( void *state, Message *msg )
{
  switch (msg->type){
  case MSG_TIMER_TIMEOUT: 
	{ return handle_timeout( msg ); }

  case MSG_VERSION_ADV:
	{
	  return handle_version_adv( msg ); 
	}

  case MSG_VERSION_DATA:
	{
	  return handle_version_data( msg );
	}

  case MSG_LOADER_INSMOD:
  case MSG_LOADER_LDMOD:
	{
	  st.type_requested = msg->type;
	  return handle_insmod( msg );
	}
  case MSG_LOADER_LDDATA:
	{
	  st.type_requested = msg->type;
	  return handle_lddata( msg );
	}
		
  case MSG_LOADER_RMMOD:
	{
	  st.type_requested = msg->type;
	  return handle_rmmod( msg );
	}
  case MSG_LOADER_RMDATA:
	{
	  st.type_requested = msg->type;
	  return handle_rmdata( msg );
	}

  case MSG_LOADER_LSDATA:
  case MSG_LOADER_LSMOD:
	{
	  st.type_requested = msg->type;
	  return handle_lsmod( msg );
	}

  case MSG_LOADER_LS_REPLY:
	{
	  return handle_lsmod_reply( msg );
	}
		
  case MSG_INIT:
	{ return handle_init(); }

  default:
	return -EINVAL;
  }
  return SOS_OK;
}

#ifdef SOS_DEBUG_DFT_LOADER
void print_mod_header(mod_header_t* mod_hdr)
{
  printf("****** MOD HEADER *************\n");
  printf("Module ID: %d\n", mod_hdr->mod_id);
  printf("State Size: %d\n", mod_hdr->state_size);
  printf("Number of timers: %d\n", mod_hdr->num_timers);
  printf("Number of sub functions: %d\n", mod_hdr->num_sub_func);
  printf("Number of prov functions: %d\n", mod_hdr->num_prov_func);
  printf("Version: %d\n", mod_hdr->version);
  printf("Processor Type: %d\n", mod_hdr->processor_type);
  printf("Platform Type: %d\n", mod_hdr->platform_type);
  printf("Code Id: %d\n", entohs(mod_hdr->code_id));
  printf("******************************\n");
  return;
}
#endif

#ifndef _MODULE_
mod_header_ptr loader_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


