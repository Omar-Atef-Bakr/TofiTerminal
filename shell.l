
/*
 *
 * CS-413 Spring 98
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
*/

%{

#include <string.h>
#include "y.tab.h"

%}

%%

\n 					{
						return NEWLINE;
					}

[ \t] 				{
						/* Discard spaces and tabs */
					}	

(?i:"EXIT") 		{	
						printf("\n		M3a alf slama\n\n");
						exit(0);
					}

([a-zA-Z0-9./]*["*"]+[a-zA-Z0-9./]*)+ {
						/* handle wildcarding */
						yylval.string_val = strdup(yytext);
						return WILDCARD;
					}

"cd"				{
						return CD;
					}
					
">>" 				{
						return GREATGREAT;
					}

">" 				{
						return GREAT;
					}

"<"					{
						return LESS;
					}

"|"					{
						return PIPE;
					}

"&"					{
						return AMP;
					}

[^ \t\n"<"">""&""|"]+ {
						/* Assume that file names have only alpha chars */
						yylval.string_val = strdup(yytext);
						return WORD;
					}

. 					{
						printf("Illegal character %c\n", *yytext);
					}

%%
	
