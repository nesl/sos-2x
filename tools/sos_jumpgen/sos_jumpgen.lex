D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"
extern FILE *yyout;
void count();
char *getbody();	
%}

%%
"/*"			{ comment(); }
"//"			{ comment_line(); }
"#"             { prepro_line(); }
"extern"        { count(0); }
"static"        { count(0); }
"inline"		{ count(0); }
"void"          { count(0); yylval.id=strdup(yytext); return(VOID); }
{L}({L}|{D})*	{ count(0); yylval.id=strdup(yytext); return(IDENTIFIER); }
"("         { count(0); return('('); }
")"         { count(0); return(')'); }
","         { count(0); return(','); }
";"         { count(0); return(';'); }
"*"         { count(0); yylval.id=strdup(yytext); return(IDENTIFIER); }
"{"         { count(0); yylval.id=getbody(); return(FUNC_BODY); }

[ \t\v\n\f]		{ count(1); }
.				{ count(1); }

%%

yywrap()
{
	return(1);
}

prepro_line()
{
	register int c;
	fprintf(yyout, "#");
	while ( (c = input()) != '\n' && c != EOF ) {
		fputc(c, yyout);    /* eat up text of comment */
	}
	fputc(c, yyout);
}

comment_line()
{
	register int c;
	fprintf(yyout, "//");
	while ( (c = input()) != '\n' && c != EOF ) {
		fputc(c, yyout);    /* eat up text of comment */
	}
	fputc(c, yyout);
}

static char *func_body = NULL;
static int  func_body_i = 0;
static int  func_size = 0;

void add_func_body(char c)
{
	if( func_body_i == func_size ) {
		func_size += 256;
		func_body = realloc(func_body, func_size);
	}

	func_body[func_body_i++] = c;
}

void eat_comment_line()
{
	register int c;
	add_func_body('/');
	add_func_body('/');
	while ( (c = input()) != '\n' && c != EOF )
	{
		add_func_body(c);
	}
	add_func_body(c);
}

comment()
{
	register int c;
	fprintf(yyout, "/*");
	for ( ; ; ) {
		while ( (c = input()) != '*' && c != EOF ) {
			fputc(c, yyout);    /* eat up text of comment */
		}

		if ( c == '*' ) { 
			fputc('*', yyout);
			while ( (c = input()) == '*' ) {
				fputc(c, yyout);
			}

			fputc(c, yyout);
			if ( c == '/' ) {
				break;    /* found the end */
			} 
		}
		if ( c == EOF ) {
			fprintf(stderr, "Error: EOF in comment" );
			break;
		}
	}
}

void eat_comment()
{
	register int c;
	add_func_body('/');
	add_func_body('*');
	for ( ; ; ) {
		while ( (c = input()) != '*' && c != EOF ) {
			add_func_body(c);
		}
		if ( c == '*' ) { 
			add_func_body(c);
			while ( (c = input()) == '*' ) {
				add_func_body( c );
			}
			add_func_body( c );
			if ( c == '/' ) {
				break;    /* found the end */
			}
		}
		if ( c == EOF ) {
			fprintf( stderr, "Error: EOF in comment" );
			break;
		}
	}
}

char *getbody()
{
	register char c;
	int depth = 1;
	char *ret;
	
	//printf("start getbody\n");
	add_func_body('{');

	for(;;) {
		c = input();
loop_again:
		if ( c == EOF ) {
			fprintf(stderr, "Error: EOF in function body" );
			break;
		} else if ( c == '{' ) {
			depth++;
		} else if( c == '}' ) {
			depth--;
			if( depth == 0 ) {
				add_func_body('}');
				add_func_body(0);
				//printf("end of getbody\n");
				//printf("%s\n", func_body);
				ret = func_body;
				func_body = NULL;
				func_size = 0;
				func_body_i = 0;
				return ret;
			}
		} else if( c == '/' ) {
			char c2 = input();
			if( c2 == '/' ) {
				eat_comment_line();
				continue;
			} else if( c2 == '*' ) {
				eat_comment();
				continue;
			} else if( c2 == '*' ) {
			} else {
				add_func_body(c);
				c = c2;
				goto loop_again;
			}
		}	
		add_func_body(c);
	}
	return NULL;	
}

int column = 0;

void count(int echo)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;

	if( echo) {
		ECHO;
	}
}


int check_type()
{
	/*
	 * pseudo code --- this is what it should check
	 *
	 *	if (yytext == type_name)
	 *		return(TYPE_NAME);
	 *
	 *	return(IDENTIFIER);
	 */

	/*
	 *	it actually will only return IDENTIFIER
	 */

	return(IDENTIFIER);
}
