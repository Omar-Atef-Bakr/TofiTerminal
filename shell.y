
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%union	{
		char   *string_val;
	}

%token <string_val> WORD

// added tokens
%token  PIPE GREATGREAT LESS AMP CD 
%token <string_val> WILDCARD
// 

%token 	NOTOKEN GREAT NEWLINE 


%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}

#define yylex yylex
#include <stdio.h>
#include <unistd.h> // for chdir
#include "command.h"
%}

%%

goal: commands
	;

commands: 
	command
	| commands command 
	;

command : simple_command
        | simple_command command // handling PIPEs
		| CD NEWLINE {
			printf("   Yacc: Change directory to home\n");
			chdir(getenv("HOME"));
			Command::_currentCommand.prompt();
		}
		| CD WORD NEWLINE {
			printf("   Yacc: Change directory to \"%s\"\n", $2);
			chdir($2);
			Command::_currentCommand.prompt();
		}
 		;

simple_command:
	// handling piped commands
    command_and_args PIPE {
		printf("   Yacc: Pipe command\n");
		Command::_currentSimpleCommand->pipe();
	}
	| command_and_args iomodifier command_end {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

command_end:
	NEWLINE
	| AMP NEWLINE {
		printf("   Yacc: Execute in background\n");
		Command::_currentCommand._background = true;
	}
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
        	printf("   Yacc: insert argument \"%s\"\n", $1);

	       	Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	| WILDCARD {
		wildcard($1);
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;
iomodifier:
	iomodifier_opt
	| iomodifier_opt iomodifier_opt
	;

iomodifier_opt:
	iomodifier_opt iomodifier_opt
	| GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREAT WORD {
		printf("   Yacc: insert append \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = true;
	}
	| LESS WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	| /* can be empty */ 
	;

%%

void wildcard(char *s)
{
	printf("   Yacc: insert wildcard \"%s\"\n", s);
	Command::_currentSimpleCommand->insertArgument(s);
	Command::_currentSimpleCommand->wildcard();
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
