%token CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE 
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
%token BOOL COMPLEX IMAGINARY RESTRICT INLINE
%token <id> IDENTIFIER
%token <id> VOID
%token <id> FUNC_BODY
%type <id> id_list

%start function_list
%union {
	char *id;
} 

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
extern FILE* yyin;
extern FILE* yyout;
char *before_func = NULL;
char *func_name = NULL;
char *var_id_list[100];
char *var_name_list[100];
char *func_body = NULL;
int num_var_names = 0;
int entry_number = 0;
int kernel_mode = 0;
int static_mode = 0;
int with_sos_call = 0;
int use_ker_get_func_ptr = 1;
int no_ker_set_current_pid = 0;
char *ker_table = NULL;
int ker_table_size_left = 0;
int ker_table_size = 0;
char* concat_id(char *id1, char *id2);
void print_var_list();
void print_typedef();
void print_func_typedef();
void print_inline();
void print_err_inline();
void print_body();
void print_func_body();
void print_func();
void printtokertable(char *string);
void printfunctokertable(char *f);
void printkertable_header();
void printkertable_tail();
void printusage();
void print_static_wrapper();
void print_func_header_define();
char get_type(char *s);
char *get_func_type();
char func_type[5];
int yydebug = 0;
%}

%%

function_list
: function { print_func(); }
| function_list function { print_func(); }
;

function 
: id_list function_name parameter_list ';' { before_func = $1; }
| id_list function_name parameter_list FUNC_BODY { before_func = $1; func_body = $4;}
| id_list function_name void_param ';'  { before_func = $1; }
| id_list function_name void_param FUNC_BODY  { before_func = $1; func_body = $4;}
;

id_list
: IDENTIFIER { $$ = $1; }
| VOID       { $$ = $1; }
| id_list IDENTIFIER  { $$ = concat_id($1, $2); }
| id_list VOID  { $$ = concat_id($1, $2); }
;

parameter
: id_list IDENTIFIER ')'  
{ 
	var_id_list[num_var_names] = $1;  
	var_name_list[num_var_names] = $2;
	num_var_names++;
}
| id_list IDENTIFIER ','  
{ 
	var_id_list[num_var_names] = $1;  
	var_name_list[num_var_names] = $2;
	num_var_names++;
}
;

parameter_list
: parameter
| parameter_list parameter
;

function_name 
: IDENTIFIER '(' { func_name = $1; }
;

void_param
: VOID ')'
| ')'
;

%%
extern char yytext[];
extern int column;

yyerror(s)
	char *s;
{
	fflush(yyout);
	fprintf(yyout, "\n%*s\n%*s\n", column, "^", column, s);
	exit(1);
}

char* concat_id(char *id1, char *id2)
{
	char *new_string = malloc(strlen(id1) + strlen(id2) + 3);

	new_string[0] = '\0';
	strcat(new_string, id1);
	strcat(new_string, " ");
	strcat(new_string, id2);
	strcat(new_string, " ");
	free(id1);
	free(id2);
	return  new_string;
}

void print_var_list()
{
	int i;
	if( num_var_names == 0 ) {
		fprintf(yyout, "void");
	}
	for( i = 0; i < num_var_names; i++) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout,"%s %s, ", var_id_list[i], var_name_list[i]);
		} else {
			fprintf(yyout,"%s %s ", var_id_list[i], var_name_list[i]);
		}
	}	
}

char ker_tail[] = "_ker_func_t";
char typedef_prefix[] = "typedef_";

void print_typedef()
{
	fprintf(yyout,"typedef %s (* %s%s)( ", before_func, func_name, ker_tail);
	print_var_list();
	fprintf(yyout,");\n");
}

void print_func_typedef()
{
	fprintf(yyout,"typedef %s (* %s%s)( ", before_func, typedef_prefix, func_name);
	print_var_list();
	fprintf(yyout,");\n");
}

void print_inline()
{
	if(kernel_mode) {
		fprintf(yyout,"static inline %s %s( ", before_func, func_name);
	} else {
		// Simon: add 'W' to the end of function 
		fprintf(yyout,"static inline %s %sDL( ", before_func, func_name);
	}
	print_var_list();
	fprintf(yyout,")\n");
}

void print_err_inline()
{
	fprintf(yyout,"static inline %s err_%s( ", before_func, func_name);
	print_var_list();
	fprintf(yyout,")\n");
}

void print_body()
{
	int i;
	fprintf(yyout,"{\n");
	fprintf(yyout,"\t%s%s func = \n\t\t(%s%s)get_kertable_entry(%d);\n", 
			func_name, ker_tail,
			func_name, ker_tail,
			entry_number);
	if(strcmp(before_func, "void") == 0 ) {
		fprintf(yyout,"\tfunc( ");
	} else {
		fprintf(yyout,"\treturn func( ");
	}
	for(i = 0; i < num_var_names; i++) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout,"%s, ", var_name_list[i]);
		} else {
			fprintf(yyout,"%s ", var_name_list[i]);
		}
	}	
	fprintf(yyout,");\n");
	fprintf(yyout,"}\n");
}

void print_func_body()
{
	int i;
	fprintf(yyout,"{\n");
	if( with_sos_call == 0 ) {
	if( no_ker_set_current_pid == 0 ) {
		fprintf(yyout, "\tsos_pid_t _prev_pid;\n");
	}
	if( use_ker_get_func_ptr != 0 ) {
		// use ker_get_func_ptr
		fprintf(yyout,"\t%s%s func = \n\t\t(%s%s) ker_get_func_ptr( %s, ", 
			typedef_prefix, func_name,
			typedef_prefix, func_name,
			var_name_list[0]);
		if( no_ker_set_current_pid == 0 ) {
			fprintf(yyout, "&_prev_pid");
		} else {
			fprintf(yyout, "NULL");
		}
		fprintf(yyout, " );\n");
	} else {
		fprintf(yyout,"\t%s%s func = \n\t\t(%s%s) sos_read_header_ptr( %s, offsetof(func_cb_t, ptr));\n", 
				typedef_prefix, func_name,
				typedef_prefix, func_name,
				var_name_list[0]);
		if( no_ker_set_current_pid == 0 ) {
			fprintf(yyout, "\t_prev_pid = ker_set_current_pid(\n");
			fprintf(yyout, "\t\tsos_read_header_byte(%s, offsetof(func_cb_t, pid)) );\n",
					var_name_list[0]);
		}
	}
	if(strcmp(before_func, "void") == 0 ) {
		fprintf(yyout, "\tfunc( ");	
	} else {
		fprintf(yyout, "\t%s ret = func( ", before_func);
	}
		
	for(i = 0; i < num_var_names; i++) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout,"%s, ", var_name_list[i]);
		} else {
			fprintf(yyout,"%s ", var_name_list[i]);
		}
	}	
	fprintf(yyout,");\n");
	if( no_ker_set_current_pid == 0 ) {
		fprintf(yyout, "\tker_set_current_pid(_prev_pid);\n");
	}
	if(strcmp(before_func, "void") != 0 ) {
		fprintf(yyout, "\treturn ret;\n");
	} 
	} else {
		if( strcmp(before_func, "void") != 0 ) {
			fprintf(yyout, "\treturn ");
		} else {
			fprintf(yyout, "\t" );
		}
		fprintf(yyout, "SOS_CALL( %s, %s%s", var_name_list[0], 
				typedef_prefix, func_name);
		if( num_var_names != 1 ) {
			fprintf(yyout, ", ");
		} else {
			fprintf(yyout, " ");
		}
		
		
		for(i = 1; i < num_var_names; i++) {
			if( i != (num_var_names - 1)) {
				fprintf(yyout,"%s, ", var_name_list[i]);
			} else {
				fprintf(yyout,"%s ", var_name_list[i]);
			}
		}	
		fprintf(yyout,");\n");
	}
	fprintf(yyout,"}\n");
}

void print_static_wrapper()
{
	int i;
	// print prototype
	fprintf(yyout,"extern %s %s( ", before_func, func_name);
	for( i = 1; i < num_var_names; i++) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout,"%s %s, ", var_id_list[i], var_name_list[i]);
		} else {
			fprintf(yyout,"%s %s ", var_id_list[i], var_name_list[i]);
		}
	}

	fprintf(yyout,");\n");

	//fprintf(yyout, "#ifdef _MODULE_\n");
	// print function name
	fprintf(yyout, "#define %sDL( ", func_name);
	for( i = 0; i < num_var_names; i++ ) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout, "%c, ", (int)'a' + i);
		} else {
			fprintf(yyout, "%c ", (int)'a' + i);
		}
	}
	fprintf(yyout,") %s(", func_name);
	for( i = 1; i < num_var_names; i++ ) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout, "(%c), ", (int)'a' + i);
		} else {
			fprintf(yyout, "(%c) ", (int)'a' + i);
		}
	}
	fprintf(yyout,")\n");

	
	/*
	print_inline();

	fprintf(yyout,"{\n");
	if(strcmp(before_func, "void") != 0 ) {
		fprintf(yyout, "\treturn ");
	} else {
		fprintf(yyout, "\t");
	}
	fprintf(yyout, "%s(", func_name);
	for(i = 1; i < num_var_names; i++) {
		if( i != (num_var_names - 1)) {
			fprintf(yyout,"%s, ", var_name_list[i]);
		} else {
			fprintf(yyout,"%s ", var_name_list[i]);
		}
	}	
	fprintf(yyout,");\n");

	fprintf(yyout,"}\n");
	//fprintf(yyout, "#endif\n");
	*/
}

char get_type(char *s)
{
	int i;
	int pointer = 0;
	char ret;
	for(i = 0; i < strlen(s); i++ ) {
		if(s[i] == '*') {
			pointer = 1;
		}
	}
	if(strncmp(s, "uint8_t", sizeof("uint8_t") - 1) == 0) {
		ret = 'C';	
	} else if(strncmp(s, "int8_t", sizeof("int8_t") - 1) == 0) {
		ret = 'c';
	} else if(strncmp(s, "uint16_t", sizeof("uint16_t") - 1) == 0) {
		ret = 'S';
	} else if(strncmp(s, "int16_t", sizeof("int16_t") - 1) == 0) {
		ret = 's';
	} else if(strncmp(s, "uint32_t", sizeof("uint32_t") - 1) == 0) {
		ret = 'L';
	} else if(strncmp(s, "int32_t", sizeof("int32_t") - 1) == 0) {
		ret = 'l';
	} else if(strncmp(s, "float", sizeof("float") - 1) == 0) {
		ret = 'f';
	} else if(strncmp(s, "double", sizeof("double") - 1) == 0) {
		ret = 'o';
	} else if(strncmp(s, "void", sizeof("void") - 1) == 0) {
		ret = 'v';
	} else {
		ret = 'y';
	} 
	if( pointer == 1) {
		ret++;
	}
	return ret;
}

char *get_func_type()
{
	int i = 0;
	char buf[1024];
	for(i = 0; i < 3; i++) {
		func_type[i] = 'v';	
	}
	if( num_var_names > 9 ) {
		func_type[3] = '9';
	} else {
		sprintf(buf, "%d", num_var_names - 1);
		//itoa(num_var_names, buf, 10);
		func_type[3] = buf[0]; 
	}
	func_type[4] = 0;
	func_type[0] = get_type(before_func);
	for( i = 1; i < num_var_names && i < 3; i++) {
		func_type[i] = get_type(var_id_list[i]);
	}
	return func_type;
}

void print_func_header_define()
{
	fprintf(yyout, "#define USE_FUNC_%s(p, d) { err_%s, \"%s\", (p), (d) }\n", 
			func_name, func_name, 
			get_func_type());
	fprintf(yyout, "#define PRVD_FUNC_%s(p, d) { %s, \"%s\", (p), (d) }\n", 
			func_name, func_name, 
			get_func_type());
}

void print_func()
{
		int i;
	if(kernel_mode) {
		if(func_body == NULL ) {
		fprintf(yyout,"\n");
		print_typedef();
		print_inline();
		print_body();
		if( func_name != NULL ) {
			printfunctokertable(func_name);
		}
		entry_number++;
		} else {
			// virtual jump table
			fprintf(yyout, "%s %s ( ", before_func, func_name);
			print_var_list();
			fprintf(yyout,")\n");
			fprintf(yyout, "%s\n", func_body);
		}
	} else {
		fprintf(yyout,"\n");
		// Generate headers for module header
		print_func_header_define();
		if( static_mode ) {
			fprintf(yyout, "#ifndef SOS_NO_LATE_BINDING\n");
		}
		print_func_typedef();
		print_inline();
		print_func_body();
		fprintf(yyout,"\n");
		print_err_inline();
		if( func_body != NULL ) {
			fprintf(yyout, "%s\n", func_body);
		} else {
			fprintf(stderr, "Error: must define error handler\n");
			exit(1);
		}
		if( static_mode ) {
			fprintf(yyout, "#else\n");
			print_static_wrapper();
			fprintf(yyout, "#endif\n");
		}
		entry_number++;
	}

	if( before_func != NULL ) {
		free(before_func);
		before_func = NULL;
	}
	if( func_name != NULL ) {
		free(func_name);
		func_name = NULL;
	}
	for( i = 0; i < num_var_names; i++) {
		free(var_id_list[i]);
		free(var_name_list[i]);
	}	
	if( func_body != NULL ) {
		free(func_body);
		func_body = NULL;
	}
	num_var_names = 0;	
	/*
	 * Add to kernel table
	 */

}

void printtokertable(char *string)
{
	if( ker_table_size_left < strlen(string) ){
		ker_table_size += (1000 + strlen(string) + 1);
		ker_table = realloc(ker_table, ker_table_size);	
		ker_table_size_left += 1000;
	} else {
		ker_table_size_left -= strlen(string);
	}
	strcat(ker_table, string);
}

void printfunctokertable(char *f)
{
	char comment_buf[100];

	sprintf(comment_buf, "/* %d */ ", entry_number);
	printtokertable(comment_buf);
	printtokertable(" (ker_table_func_t) ");
	printtokertable(f);
	printtokertable(",\t\t\t\t\\\n");
}

void printkertable_header()
{
	printtokertable("typedef void (*ker_table_func_t)(void);\n");
	printtokertable("#define SOS_KER_TABLE(...) {                  \\\n");
}

void printkertable_tail()
{
	printtokertable("}\n");
	//fprintf(yyout, "%s", ker_table);
}

void printusage()
{
	printf("SOS Jump Table Generator Command Line Usage:\n");
	printf("sos_jumpgen [-k <starting entry number>] [-o <Output File Name>] [-h] input_file1, input_file2, ...\n");
	printf(" -k                    kernel mode.  Generates kernel jump table\n");
	printf(" -s                    generate static wrapper (experimental)\n");
	printf(" -n                    Using sos_read_header_ptr instead of ker_get_func_ptr\n");
	printf(" -r                    Don't generate ker_set_current_pid()\n");
	printf(" -c                    Generate SOS_CALL \n");
	printf(" -o <Output File Name> If not set, default to stdout\n");
	printf(" -h                    print this help\n");
}

int main( int argc, char **argv ) 
{
	int ch;
	int i;
	//yydebug=1;

	yyin = stdin;
	yyout = stdout;
	//optind = 1;
	//printf("argc = %d\n", argc);
	while( 1 ) {
		ch = getopt(argc, argv, "k:rcshno:");
		//printf("getopt = %d\n", ch);
		if( ch == -1 ) break;
		switch ( ch ) {
		case 'o':
			yyout = fopen(optarg, "w");
			if( yyout == NULL ) {
				fprintf(stderr, "cannot write to %s\n", optarg);
				exit( 1 );
			}
			break;
		case 'k':
			kernel_mode = 1;
			if( sscanf(optarg, "%d", &entry_number) != 1 ) {
				fprintf(stderr, "%s is not a valid entry number\n", optarg);
				exit( 1 );
			}
			break;
		case 's':
			static_mode = 1;
			break;
		case 'n':
			use_ker_get_func_ptr = 0;	
			break;
		case 'r':
			no_ker_set_current_pid = 1;
			break;
		case 'c':
			with_sos_call = 1;
			break;
		case '?':	
		case 'h':
				printusage();
				exit( 0 );
		}
	}
	//printf("optind = %d\n", optind);
	argc -= optind;
	argv += optind;
	printkertable_header();
	if( argc == 0 ) {
		yyparse();
	} else {
	for( i = 0; i < argc; i++) {
		before_func = NULL;
		func_name = NULL;
		num_var_names = 0;

		yyin = fopen(argv[i], "r");
		if( yyin == NULL ) {
			fprintf(stderr, "Cannot find %s\n", argv[i]);
			exit(1);
		}
		yyparse();
		fclose(yyin);
	}
	}
	printkertable_tail();
	
	fflush(yyout);
	if( yyout != stdout) {
		fclose(yyout);
	}
	return 0;
}

