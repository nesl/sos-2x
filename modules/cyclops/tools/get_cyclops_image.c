/**
 * \file get_cyclops_image.c
 * \brief Get the image from Cyclops
 */



#include <camera_settings.h>
#include <stdio.h>
#include <string.h>
#include <sossrv_client.h>
#include <mod_pid.h>
#include <radioDump.h>
#include <serialDump.h>
#include <platform/cyclops/include/plat_msg_types.h>
#include <processor/avr/include/pid_proc.h>
#include <platform/cyclops/include/pid_plat.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <signal.h>

//----------------------------------------
// TYPEDEFS
//----------------------------------------


#define RAW_IMAGE_BUFFER_SIZE _CYCLOPS_COLOR_DEPTH_*_CYCLOPS_RESOLUTION_H*_CYCLOPS_RESOLUTION_W

#define NACK_TIMER 0
#define ACK_TIMER 1

typedef struct raw_image_s
{
  uint32_t size;
  uint8_t rawImageBuff[RAW_IMAGE_BUFFER_SIZE];  
} raw_image_t;

typedef struct HEADER{
  unsigned short int type;                 /* Magic identifier            */
  unsigned int size;                       /* File size in bytes          */
  unsigned short int reserved1, reserved2;
  unsigned int offset;                     /* Offset to image data, bytes */
}  __attribute__((packed)) HEADER;

typedef struct INFOHEADER {
  unsigned int size;               /* Header size in bytes      */
  int width,height;                /* Width and height of image */
  unsigned short int planes;       /* Number of colour planes   */
  unsigned short int bits;         /* Bits per pixel            */
  unsigned int compression;        /* Compression type          */
  unsigned int imagesize;          /* Image size in bytes       */
  int xresolution,yresolution;     /* Pixels per meter          */
  unsigned int ncolours;           /* Number of colours         */
  unsigned int importantcolours;   /* Important colours         */
} INFOHEADER;


//----------------------------------------
// STATIC GLOBAL VARIABLES
//----------------------------------------
uint8_t fileImageBuff[RAW_IMAGE_BUFFER_SIZE];
raw_image_t rawImg;
uint8_t* rawBuffPtr;
uint8_t* fileDumpPtr;
uint32_t NumBytesRx;
uint32_t TotalSize;
uint32_t pictureCount;
uint8_t packetReceiverStatus;
uint16_t lastSeqRcv;
uint16_t sequenceOut ;
uint8_t timerState ;
framCount_t framesRcvd;

time_t cur_time;

struct itimerval interval;
	
enum {IDLE, BUSY};

//----------------------------------------
// PROTOTYPES
//----------------------------------------
static int msg_from_cyclops_handler(Message* psosmsg);
static void defrag_radio_dump(radioDumpFrame_t *s);
static void InitRawImg();
static void print_dump(uint8_t *myBuf, uint32_t myBufSize);
static void print_file_dump(uint8_t *myBuf, uint32_t myBufSize);
static void generateBitMap(uint8_t *myBuf, uint32_t myBufSize);
static void timer_timeout(int signal);
static void binary_print(uint32_t input);
static void payload_print(uint8_t *input);

//----------------------------------------
int main(int argc, char *argv[])
{
  if (argc < 3){
    printf("Usage Error!\n");
    printf("get_cyclops_image <server address> <server port>\n");
    return -1;
  }
  packetReceiverStatus = IDLE;
  pictureCount = 0;
  // Connect to the SOS server
  sossrv_connect(argv[1], argv[2]);

  // Setup the handler to receive SOS Messages
  if (sossrv_recv_msg(msg_from_cyclops_handler) < 0){
    printf("Setting up handler before connecting.\n");
  }

  InitRawImg();
   
  // Loop forever
  while(1){sleep(10);};
  return 0;
}  

//----------------------------------------------------
// MESSAGE HANDLER
//----------------------------------------------------
static int msg_from_cyclops_handler(Message* psosmsg)
{
  int i;
  if ( (psosmsg->sid==SERIAL_DUMP_PID) || (psosmsg->sid==RADIO_DUMP_PID) )
    {
      if (psosmsg->type == MSG_RAW_IMAGE_FRAGMENT) 
	{
	  defrag_radio_dump((radioDumpFrame_t*)psosmsg->data);
	}
	
      else if  (psosmsg->type == MSG_DATA_ACK_BACK) 
	{			
	  //Kill the timer
			
	  printf("Received ACK_BACK time =%s\n", ctime(&cur_time));	
	  interval.it_interval.tv_sec = 1;
	  interval.it_interval.tv_usec = 0;
	  interval.it_value.tv_sec = 0;
	  interval.it_value.tv_usec = 0;  
	  signal(SIGALRM, timer_timeout);				
	  (void) setitimer(ITIMER_REAL , &interval, NULL);
	  cur_time = time(NULL);
	  printf("TCP Connection Ended time =%s\n", ctime(&cur_time));	
	  InitRawImg();  
	}
    }
  return 0;  
}

//----------------------------------------------------
// RADIO DUMP DE-FRAGMENT
//----------------------------------------------------
static void defrag_radio_dump(radioDumpFrame_t *s)
{
  uint16_t tempseq;
		
	
  // Fix the endian-ness
  tempseq = s->seq;
  s->seq = entohs(tempseq);
		
	
  // stop the timer and send an ack or nack 
	 			
  interval.it_interval.tv_sec = 1;
  interval.it_interval.tv_usec = 0;
  interval.it_value.tv_sec = 0;
  interval.it_value.tv_usec = 0;  
  signal(SIGALRM, timer_timeout);
  (void) setitimer(ITIMER_REAL , &interval, NULL);

	 
  //printf("frame sequence: %d\n",s->seq);
	
  uint8_t *payloadPtr = &(s->payload[0]);
	
  payload_print(payloadPtr);
	
	
  //check off the receivbed bits
  /* if (s->seq >= 32)	//upper 32 bits
     {
     uint8_t frameSequence = s->seq - 32;
     uint32_t mask = 1;
     mask  <<=  frameSequence;
     framesRcvd.frameSequence[1] &= ~mask;	  
     }
     else
     { 
     uint8_t frameSequence = s->seq ;
     uint32_t mask = 1;
     mask  <<=  frameSequence;
     framesRcvd.frameSequence[0] &= ~mask;	  	 
     }*/
	 
  // fill in the dump buffer
  if(s->seq < RAW_IMAGE_BUFFER_SIZE/UART_PAYLOAD_LEN) //catch errors
    memcpy(fileDumpPtr+(RAW_IMAGE_BUFFER_SIZE - ((s->seq+1) * UART_PAYLOAD_LEN)), s->payload, UART_PAYLOAD_LEN);
  //	memcpy(rawBuffPtr, s->payload, UART_PAYLOAD_LEN);
  //      NumBytesRx += UART_PAYLOAD_LEN;

    
    
    
  printf("frame sequence: %d\n",s->seq);

	
  //binary_print(framesRcvd.frameSequence[1]);
  //printf(" ");
  //binary_print(framesRcvd.frameSequence[0]);		
  //printf("\n");

  {		
    timerState = NACK_TIMER;
    interval.it_interval.tv_sec = 1;
    interval.it_interval.tv_usec = 0;
    interval.it_value.tv_sec = 1;
    interval.it_value.tv_usec = 0;  
    signal(SIGALRM, timer_timeout);	
    (void) setitimer(ITIMER_REAL , &interval, NULL);
    cur_time = time(NULL);
    printf("starting NACK time =%s\n", ctime(&cur_time));	
		
  }		
}

static void payload_print(uint8_t *input)
{
  uint8_t count = 0;
  while (count < UART_PAYLOAD_LEN)
    {
      printf("%X ", input[count]);
      count++;
    }
  printf("\n");
}

static void binary_print(uint32_t input)
{
  uint32_t mask = 0x80000000;
  uint8_t count = 0;
  while (count < 32)
    {
      if ( mask & input )
	{
	  printf("1");
	}
      else
	{
	  printf("0");
	}
      mask >>=1;
      count++;
    }
}
static void timer_timeout(int signalResult)
{
  //time_has_come = 1;
		
  if (timerState == NACK_TIMER)
    {	
		
      /********************** KEVin NO TCP****************/
      printf("\n");
      //printf("Image Size: %d\n", NumBytesRx);
      printf("Done Receiving Image\n");
      //rawImg.size = NumBytesRx;
		      
      generateBitMap(fileDumpPtr, RAW_IMAGE_BUFFER_SIZE);
      print_file_dump(fileDumpPtr, RAW_IMAGE_BUFFER_SIZE);   
		
    }
	
	
  interval.it_interval.tv_sec = 1;
  interval.it_interval.tv_usec = 0;
  interval.it_value.tv_sec = 0;
  interval.it_value.tv_usec = 0;  
  signal(SIGALRM, timer_timeout);
  (void) setitimer(ITIMER_REAL , &interval, NULL);
	
	
  return;
}
	
static void InitRawImg()
{
  lastSeqRcv = 0;
  rawImg.size = 0;
  bzero(rawImg.rawImageBuff, RAW_IMAGE_BUFFER_SIZE); 
  rawBuffPtr = &(rawImg.rawImageBuff[0]);
  NumBytesRx = 0;
  TotalSize = 0;
	
  framesRcvd.frameSequence[0] = 0xFFFFFFFF;
  framesRcvd.frameSequence[1] = 0xFFFFFFFF;

  binary_print(framesRcvd.frameSequence[1]);
  printf(" ");
  binary_print(framesRcvd.frameSequence[0]);		
  printf("\n");
	
  bzero(fileImageBuff, RAW_IMAGE_BUFFER_SIZE); 
  fileDumpPtr = &(fileImageBuff[0]);
  bzero(fileDumpPtr, RAW_IMAGE_BUFFER_SIZE);

	
}


//----------------------------------------------------
// IMAGE PRINT DUMP
//----------------------------------------------------
void print_dump(uint8_t *myBuf, uint32_t myBufSize)
{
  int i;
  printf("\n\n*****************radio dump******************\n");
  for(i=0;i<myBufSize; i++) 
    {
      if(i%16==0) printf("\n");
      printf("%3d ", (unsigned char) myBuf[i]);
    }
  printf("\n********************end*********************\n");fflush(stdout);
  return;
}


//----------------------------------------------------
// IMAGE FILE DUMP
//----------------------------------------------------
void print_file_dump(uint8_t *myBuf, uint32_t myBufSize)
{
  int i;
  FILE *fp1,*fp2;	
	
  if((fp1 = fopen ("frame.dat", "w"))==NULL) printf("Data file error\n");
  for(i=0;i<myBufSize; i++) 
    {
      if(i%16==0 && i!=0) fprintf(fp1,"\n");
      fprintf(fp1,"%3d ", (uint8_t) myBuf[i]);
    }
  fclose(fp1);
  //we only create lock file
  fp2 = fopen (".frame.Lock", "w");
  fclose(fp2);
  return;
}


//----------------------------------------------------
// STORE BITMAP FILE FORMAT
//----------------------------------------------------
/***************************************************/
// _CYCLOPS_RESOLUTION_ x _CYCLOPS_RESOLUTION_
/***************************************************/
#if _CYCLOPS_COLOR_DEPTH_ == 1 //B&W
void generateBitMap(uint8_t *myBuf, uint32_t myBufSize)
{
      
  FILE *myFP;
  HEADER h1;
  INFOHEADER h2;
  char filename[30];
  int zero=0;
  int i;
  char j=0;


  sprintf(filename,"frame%d.bmp",pictureCount);
  printf( "Save in a Bitmap File %s",filename);      


  if((myFP = fopen (filename, "wb"))==NULL){
    printf("Data file error\n");
  }
     
  h1.type = entohs(19778); //'M' * 256 + 'B';
  h1.size = entohl(myBufSize+54+256*4);
  h1.reserved1 = entohs(0);
  h1.reserved2 = entohs(0);
  h1.offset = entohl(54+256*4);
      
  h2.size = entohl(40);
  h2.width = entohl(_CYCLOPS_RESOLUTION_W);
  h2.height = entohl(-1* _CYCLOPS_RESOLUTION_H);
  h2.planes = entohs(1);
  h2.bits = entohs(8);
  h2.compression = entohl(0);
  h2.imagesize = entohl(0);
  h2.xresolution = entohl(0);
  h2.yresolution = entohl(0);
  h2.ncolours = entohl(256);
  h2.importantcolours = entohl(0);
           
  //write out headers 
  fwrite (&h1, sizeof(HEADER), 1, myFP);
  fwrite (&h2, sizeof(INFOHEADER), 1, myFP);

  /*
   *generate the grayscale pallette.  
   * This is needed because bmp format doesnt
   * support gray scale, so convention has it 
   * that you use 256 color and make all 256 colors gray
   */

  for(i=0; i<256; i++)
    {
      j++;
      fwrite (&j, sizeof(char), 1, myFP);
      fwrite (&j, sizeof(char), 1, myFP);
      fwrite (&j, sizeof(char), 1, myFP);
      fwrite (&zero, sizeof(char), 1, myFP);
    }

  //write out picture
  fwrite (myBuf, sizeof(char), RAW_IMAGE_BUFFER_SIZE, myFP);

  fclose(myFP);
  pictureCount++;
}
#elif _CYCLOPS_COLOR_DEPTH_ == 2 //YCbCr
void generateBitMap(uint8_t *myBuf, uint32_t myBufSize)
{
      
  FILE *myFP;
  HEADER h1;
  INFOHEADER h2;
  char filename[30];
  int i;
  int 0x00 j;
  uint8_t *new_buf;
  int newBufSize=myBufSize/2*3;
  new_buf=malloc(RAW_IMAGE_BUFFER_SIZE/2*3);


  j=0;
  for(i=0; i<RAW_IMAGE_BUFFER_SIZE; i+=4)//hopefully converting from yCbCr to RGB
    {					//data comes in YCbYCr
      new_buf[j++]=1.164*(myBuf[i])+1.403*(myBuf[i+1]-128);	//R
      new_buf[j++]=1.164*(myBuf[i])
	-0.344*(myBuf[i+3]-128) - 0.714*(myBuf[i+1]-128);//G
      new_buf[j++]=1.164*(myBuf[i])+ 1.77*(myBuf[i+3]-128);	//B
      new_buf[j++]=1.164*(myBuf[i+2])+1.403*(myBuf[i+1]-128);	//R
      new_buf[j++]=1.164*(myBuf[i+2])
	-0.344*(myBuf[i+3]-128) - 0.714*(myBuf[i+1]-128);//G
      new_buf[j++]=1.164*(myBuf[i+2])+ 1.773*(myBuf[i+3]-128);	//B
    }


  sprintf(filename,"frame%d.bmp",pictureCount);
  printf( "Save in a Bitmap File %s",filename);      


  if((myFP = fopen (filename, "wb"))==NULL){
    printf("Data file error\n");
  }
     
  h1.type = entohs(19778); //'M' * 256 + 'B';
  h1.size = entohl(newBufSize+54);
  h1.reserved1 = entohs(0);
  h1.reserved2 = entohs(0);
  h1.offset = entohl(54);
      
  h2.size = entohl(40);
  h2.width = entohl(_CYCLOPS_RESOLUTION_W);
  h2.height = entohl(-1* _CYCLOPS_RESOLUTION_H);
  h2.planes = entohs(1);
  h2.bits = entohs(24);
  h2.compression = entohl(0);
  h2.imagesize = entohl(0);
  h2.xresolution = entohl(0);
  h2.yresolution = entohl(0);
  h2.ncolours = entohl(0);
  h2.importantcolours = entohl(0);
            
  fwrite (&h1, sizeof(HEADER), 1, myFP);
  fwrite (&h2, sizeof(INFOHEADER), 1, myFP);

  /*
   *writing out the data
   *bmp must be 4 byte aligned so padding is added to each line as needed
   */
  fwrite (new_buf,sizeof(char), newBufSize, myFP);
  fclose(myFP);
  pictureCount++;
}


#elif _CYCLOPS_COLOR_DEPTH_ == 3 //rgb
void generateBitMap(uint8_t *myBuf, uint32_t myBufSize)
{
      
  FILE *myFP;
  HEADER h1;
  INFOHEADER h2;
  char filename[30];
  int zero=0;
  int i; 
  char j=0;
  int channel_size=RAW_IMAGE_BUFFER_SIZE/3;


  sprintf(filename,"frame%d.bmp",pictureCount);
  printf( "Save in a Bitmap File %s",filename);      


  if((myFP = fopen (filename, "wb"))==NULL){
    printf("Data file error\n");
  }
     
  h1.type = entohs(19778); //'M' * 256 + 'B';
  h1.size = entohl(myBufSize+54);
  h1.reserved1 = entohs(0);
  h1.reserved2 = entohs(0);
  h1.offset = entohl(54);
      
  h2.size = entohl(40);
  h2.width = entohl(_CYCLOPS_RESOLUTION_W);
  h2.height = entohl(-1* _CYCLOPS_RESOLUTION_H);//negative make it like screen cordinates
  h2.planes = entohs(1);
  h2.bits = entohs(24);
  h2.compression = entohl(0);
  h2.imagesize = entohl(0);
  h2.xresolution = entohl(0);
  h2.yresolution = entohl(0);
  h2.ncolours = entohl(0);
  h2.importantcolours = entohl(0);
            
  fwrite (&h1, sizeof(HEADER), 1, myFP);
  fwrite (&h2, sizeof(INFOHEADER), 1, myFP);

  for(i=0; i<myBufSize;i+=3)
    {
      fwrite (myBuf+i+2,sizeof(char), 1, myFP);//camera is big endian
      fwrite (myBuf+i+1,sizeof(char), 1, myFP);//so we flip endian ness here 
      fwrite (myBuf+i,sizeof(char), 1, myFP); //this shoud have code to check if it is a ppc or not (currently assumes it a little endian box
    }
  fclose(myFP);
  pictureCount++;
}

#endif



