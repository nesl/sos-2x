%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <VM/codesend.h>
#include <sos_endian.h>
#include <pid.h>
#include <mod_pid.h>

#define MSG_TIMER_TIMEOUT 2
#define TIMER_PID 4
#define DVM_MAX_SCRIPT_LENGTH 255
#define DVM_SCRIPT_HEADER_SIZE 7

typedef struct {
	unsigned char destModID;		// lddata requirement: destination module ID
	unsigned char capsuleID;		// lddata requirement: data ID
	unsigned char moduleID;
	unsigned char eventType;
	unsigned short length;
	unsigned char libraryMask;
	unsigned char data[DVM_MAX_SCRIPT_LENGTH];
} __attribute__((packed)) DvmScript;

unsigned char *data_ptr;
int local = 0;
unsigned int i, inc;
DvmScript script;

%}


%%
INC			{inc = 1;}
MATH		{if (inc) script.libraryMask |= 1;} 
EWMALIBDIR	{if (inc) script.libraryMask |= 2;}
EWMALIB		{if (inc) script.libraryMask |= 1;}
OUTLIERLIBDIR	{if (inc) script.libraryMask |= 2;}
OUTLIERLIB	{if (inc) script.libraryMask |= 1;}
RAGO		{if (inc) script.libraryMask |= 2;}
CYCLOPS		{if (inc) script.libraryMask |= 4;}
HALT	{if (inc) {inc = 0;} data_ptr[i++] = OP_HALT;}
START	{if (inc) {inc = 0;} data_ptr[i++] = OP_START;}
STOP	{if (inc) {inc = 0;} data_ptr[i++] = OP_STOP;}
NOP		{if (inc) {inc = 0;} data_ptr[i++] = OP_NOP;}
OUTLIER	{if (inc) {inc = 0;} data_ptr[i++] = OP_OUTLIER;}
OUTDIR	{if (inc) {inc = 0;} data_ptr[i++] = OP_OUTDIR;}
PUSH	{if (inc) {inc = 0;} data_ptr[i++] = OP_PUSH;}
PUSHF	{if (inc) {inc = 0;} data_ptr[i++] = OP_PUSHF;}
POP		{if (inc) {inc = 0;} data_ptr[i++] = OP_POP;}
LED		{if (inc) {inc = 0;} data_ptr[i++] = OP_LED;}
RAND	{if (inc) {inc = 0;} data_ptr[i++] = OP_RAND;}
GETLOCALF {if (inc) {inc = 0;} data_ptr[i] = OP_GETLOCALF; local = 1;}
SETLOCALF {if (inc) {inc = 0;} data_ptr[i] = OP_SETLOCALF; local = 1;}
GETLOCAL {if (inc) {inc = 0;} data_ptr[i] = OP_GETLOCAL; local = 1;}
SETLOCAL {if (inc) {inc = 0;} data_ptr[i] = OP_SETLOCAL; local = 1;}
GETVARF {if (inc) {inc = 0;} data_ptr[i] = OP_GETVARF; local = 1;}
SETVARF {if (inc) {inc = 0;} data_ptr[i] = OP_SETVARF; local = 1;}
GETVAR {if (inc) {inc = 0;} data_ptr[i] = OP_GETVAR; local = 1;}
SETVAR {if (inc) {inc = 0;} data_ptr[i] = OP_SETVAR; local = 1;}
SETTIMER	{if (inc) {inc = 0;} data_ptr[i] = OP_SETTIMER; local = 1;}
JNZ	{if (inc) {inc = 0;} data_ptr[i++] = OP_JNZ;}
JZ	{if (inc) {inc = 0;} data_ptr[i++] = OP_JZ;}
JGE	{if (inc) {inc = 0;} data_ptr[i++] = OP_JGE;}
JG	{if (inc) {inc = 0;} data_ptr[i++] = OP_JG;}
JLE	{if (inc) {inc = 0;} data_ptr[i++] = OP_JLE;}
JL	{if (inc) {inc = 0;} data_ptr[i++] = OP_JL;}
JE	{if (inc) {inc = 0;} data_ptr[i++] = OP_JE;}
JNE	{if (inc) {inc = 0;} data_ptr[i++] = OP_JNE;}
GET_DATA {if (inc) {inc = 0;} data_ptr[i] = OP_GET_DATA; local = 1;}
POST	{if (inc) {inc = 0;} data_ptr[i++] = OP_POST;}
BCAST	{if (inc) {inc = 0;} data_ptr[i++] = OP_BCAST;}
POSTNET	{if (inc) {inc = 0;} data_ptr[i++] = OP_POSTNET;}
BPUSH	{if (inc) {inc = 0;} data_ptr[i] = OP_BPUSH; local = 1;}
BAPPEND	{if (inc) {inc = 0;} data_ptr[i] = OP_BAPPEND; local = 1;}
BREADF	{if (inc) {inc = 0;} data_ptr[i++] = OP_BREADF;}
EWMADIR	{if (inc) {inc = 0;} data_ptr[i++] = OP_EWMADIR;}
EWMA	{if (inc) {inc = 0;} data_ptr[i++] = OP_EWMA;}
CALL	{if (inc) {inc = 0;} data_ptr[i++] = OP_CALL; }
JMP		{if (inc) {inc = 0;} data_ptr[i++] = OP_JMP; }
BCLEAR	{if (inc) {inc = 0;} data_ptr[i++] = OP_BCLEAR; }
BSET	{if (inc) {inc = 0;} data_ptr[i++] = OP_BSET; }
BINIT	{if (inc) {inc = 0;} data_ptr[i++] = OP_BINIT; }
BZERO	{if (inc) {inc = 0;} data_ptr[i++] = OP_BZERO; }
ABS		{if (inc) {inc = 0;} data_ptr[i++] = OP_ABS;}
ADD		{if (inc) {inc = 0;} data_ptr[i++] = OP_ADD;}
MULT	{if (inc) {inc = 0;} data_ptr[i++] = OP_MUL;}
SUB		{if (inc) {inc = 0;} data_ptr[i++] = OP_SUB;}
DIV		{if (inc) {inc = 0;} data_ptr[i++] = OP_DIV;}
MOD		{if (inc) {inc = 0;} data_ptr[i++] = OP_MOD;}
INCR	{if (inc) {inc = 0;} data_ptr[i++] = OP_INCR;}
DECR	{if (inc) {inc = 0;} data_ptr[i++] = OP_DECR;}
MOVE 	{if (inc) {inc = 0;} data_ptr[i++] = OP_MOVE;}
TURN	{if (inc) {inc = 0;} data_ptr[i++] = OP_TURN;}
PAN	{if (inc) {inc = 0;} data_ptr[i++] = OP_PAN;}
TILT	{if (inc) {inc = 0;} data_ptr[i++] = OP_TILT;}
SCAN	{if (inc) {inc = 0;} data_ptr[i++] = OP_SCAN;}
SHOW_VALUE	{if (inc) {inc = 0;} data_ptr[i++] = OP_SHOW_VALUE;}
FIND_DISTANCE	{if (inc) {inc = 0;} data_ptr[i++] = OP_FIND_DISTANCE;}
SALUTE	{if (inc) {inc = 0;} data_ptr[i++] = OP_SALUTE;}
FLASHLED	{if (inc) {inc = 0;} data_ptr[i++] = OP_FLASHLED;}
ENABLE_CLIFF	{if (inc) {inc = 0;} data_ptr[i++] = OP_ENABLE_CLIFF;}
ENABLE_PUSHBUTTON {if (inc) {inc = 0;} data_ptr[i++] = OP_ENABLE_PUSHBUTTON;}
PHOTO  { if (local) {
		data_ptr[i++] += 0;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
TEMPERATURE  { if (local) {
		data_ptr[i++] += 1;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
MIC  { if (local) {
		data_ptr[i++] += 2;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
ACCELX  { if (local) {
		data_ptr[i++] += 3;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
ACCELY  { if (local) {
		data_ptr[i++] += 4;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
MAGX  { if (local) {
		data_ptr[i++] += 5;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }
MAGY  { if (local) {
		data_ptr[i++] += 6;
		local = 0;
	 } else
		data_ptr[i++] = 0;
       }

[\-]*[0-9]+	{ if (!local) { 
		data_ptr[i++] = atoi(yytext);
	  } else {
		data_ptr[i] += atoi(yytext);
		i++;
		local = 0;
	 }
	}
[ \t\n]	//printf(" ");


HEADLIGHTS_ON	data_ptr[i++] = 0;
HEADLIGHTS_OFF	data_ptr[i++] = 1;
DECKLIGHTS_ON	data_ptr[i++] = 2;
DECKLIGHTS_OFF	data_ptr[i++] = 3;
BARGRAPH_ON		data_ptr[i++] = 4;
BARGRAPH_OFF	data_ptr[i++] = 5;
BLUELED_ON		data_ptr[i++] = 6;
BLUELED_OFF		data_ptr[i++] = 7;

FALSE		data_ptr[i++] = 0;
TRUE		data_ptr[i++] = 1;


%%

main( argc, argv )
int argc;
char **argv;
{
	int temp;
	
	script.libraryMask = 0;
	data_ptr = script.data;	
	i = 0;
	inc = 1;
	
	if (argc < 3)
	{
		printf("Usage Error!\n");
		printf("dvm_compiler <filename> <capsule name> [<event module> <event type>]\n");
		return 1;
	}

	script.destModID = DVM_MODULE_PID;
	script.capsuleID = DVM_CAPSULE_INVALID;
	if( argc >= 4 ) {
		script.moduleID = atoi(argv[3]);
	} else {
		script.moduleID = 0;
	}
	if( argc >= 5 ) {
		script.eventType = atoi(argv[4]);
	} else {
		script.eventType = 0;
	}
	if (strcmp(argv[2], "timer3") == 0) {
		script.capsuleID = DVM_CAPSULE_ID6;
		script.moduleID = TIMER_PID;
		script.eventType = MSG_TIMER_TIMEOUT;	
	} else if (strcmp(argv[2], "timer2") == 0) {
		script.capsuleID = DVM_CAPSULE_ID5;
		script.moduleID = TIMER_PID;
		script.eventType = MSG_TIMER_TIMEOUT;	
	} else if (strcmp(argv[2], "timer1") == 0) {
		script.capsuleID = DVM_CAPSULE_ID4;
		script.moduleID = TIMER_PID;
		script.eventType = MSG_TIMER_TIMEOUT;	
	} else if (strcmp(argv[2], "timer0") == 0) {
		script.capsuleID = DVM_CAPSULE_ID1;
		script.moduleID = TIMER_PID;
		script.eventType = MSG_TIMER_TIMEOUT;	
	} else if (strcmp(argv[2], "pushbutton") == 0)
		script.capsuleID = DVM_CAPSULE_ID2;
	else if (strcmp(argv[2], "reboot") == 0)
		script.capsuleID = DVM_CAPSULE_REBOOT;
	else if (strcmp(argv[2], "cliff") == 0)
		script.capsuleID = DVM_CAPSULE_ID3;
	else exit(printf("Wrong capsule name\n"));

	//open file	
	yyin = fopen( argv[1], "r" );
	if(yyin == NULL)
	{
		printf("Data file could not be opened\n");
		exit(0);
	}
		
	yylex();
	
	script.length = ehtons(i);

	printf("// %s context \n", argv[2]);
	/*
	for(temp = 0; temp < i; temp++) 
		printf("%x ", script.data[temp]);
	printf("\n");
	*/
	printf("uint8_t %s_script_buf[%d] = { ", argv[2], DVM_SCRIPT_HEADER_SIZE + i);
	{
		FILE *fd;
	fd = fopen("script.dat", "w");
	if (fd < 0) exit(printf("Could not open script.dat\n"));
	
	{
		int write_idx;

		for(write_idx = 0; write_idx < (DVM_SCRIPT_HEADER_SIZE + i); write_idx++)
		{
			unsigned char *ptr = (unsigned char*)&script;
			fprintf(fd, "0x%x ", *(ptr + write_idx));
			printf("0x%x,", *(ptr + write_idx));
		}
	}
	printf(" };\n");
	printf("load_script(%s_script_buf, %d);\n", argv[2], DVM_SCRIPT_HEADER_SIZE + i);
	fflush(fd);
	//write(fd, (void *)&script, DVM_SCRIPT_HEADER_SIZE + i);
	
	fclose(fd);
	}
	
	if ((DVM_SCRIPT_HEADER_SIZE + i) > 256)
		exit(printf("Script packet size is greater than 256 bytes. Exiting.\n"));
	
}
