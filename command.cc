
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "command.h"

#define DEBUG 0
#define LOGFILE "logs.txt"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
	_pipe = false;
	_wildcard = false;
}

void
SimpleCommand::insertArgument( char * argument )
{
	// realocate space if necessary
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	// Insert argument
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

// added functions
void SimpleCommand::pipe(){
	_pipe = true;
}

void SimpleCommand::wildcard(){
	_wildcard = true;
}
// end of added functions

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;

	// potential bug: changed numberOfSimpleCommands to numberOfAvailableSimpleCommands 
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = false;
}


void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	// potential bug: add 1 to the if statment range
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = false;
	
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

bool children_reaped = false;

void sigchld_handler(int sig) {

	// open logs
	FILE *logfile = fopen(LOGFILE, "a+");
	if (logfile == NULL) {
		perror("Error opening log file");
		return;
	}

	// reap all children
	pid_t pid;
    int   status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
		fprintf(logfile, "Child process with PID %d terminated with status %d\n", pid, status);
	}

	// close logs
	fclose(logfile);

	// set flag
	children_reaped = true;

}

void
Command::execute()
{
	signal(SIGCHLD, sigchld_handler);

	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();
	// Add execution here
	
	// create pipe
	int fdpipe[2]; // [5, 6]
	if(pipe(fdpipe) == -1){
		perror("pipe");
		exit(2);
	}

	// save default in/out/err
	int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);

	// For every simple command fork a new process
	for(int i=0; i<_numberOfSimpleCommands; i++){
        // reset i/o
        dup2( defaultin, 0 );
        dup2( defaultout, 1 );
        dup2( defaulterr, 2 );

        if(DEBUG){
            printf("\nsimple command %d:\n\n", i);
        }

		SimpleCommand *current = _simpleCommands[i];

        // handle input file
        if(_inputFile){
			if(DEBUG){
				printf("input file: %s\n", _inputFile);
			}
            int fd = open(_inputFile, O_RDONLY);
            dup2(fd, 0);
            close(fd);
        }

        // if prev child pipe is on, redirect input to pipe
        if(i > 0 && _simpleCommands[i-1]->_pipe){
            if (DEBUG)
                printf("inpipe is on\n");

            dup2(fdpipe[0], 0);

            // close pipe
            close(fdpipe[0]);
        }

        // handle output
        if(_outFile){
			if(DEBUG){
				printf("output file: %s\n", _outFile);
			}

            // set mode to write only and create if necessary
            auto mode = O_CREAT | O_WRONLY;

            // if append mode is on, set mode to append as well
            if (_append){
                mode |= O_APPEND;
            }
                // else set mode to truncate (overwrite)
            else{
                mode |= O_TRUNC;
            }

            int fd = open(_outFile, mode);
            dup2(fd, 1);
            close(fd);
        }

        // if pipe is on, redirect stdout to pipe
        if(current->_pipe){
            if (DEBUG){
                printf("pipe is on\n");
            }
            // open new pipe
            if(pipe(fdpipe) == -1){
                perror("pipe");
                exit(2);
            }
            dup2(fdpipe[1], 1);

            // close writing end of the pipe
            close(fdpipe[1]);
        }

        if(_errFile){
            int fd = open(_errFile, O_CREAT | O_WRONLY);
            dup2(fd, 2);
            close(fd);
        }

        pid_t pid = fork();
        if(pid == 0){
            // child process

            // close all
            close(fdpipe[0]);
            close(fdpipe[1]);
			close(defaultin);
			close(defaultout);
			close(defaulterr);

			// execute
			if(current->_wildcard){
				char* command;
				command = (char*)malloc(100*sizeof(char));

				strcpy(command, current->_arguments[0]);
				for (int i = 1; i < current->_numberOfArguments; i++){
					strcat(command, " ");
					strcat(command, current->_arguments[i]);
				}
				system(command);
				exit(0);
			}
			else{
				execvp(current->_arguments[0], current->_arguments);
				// blow up if child doesn't suicide
				perror("execvp");
				exit(1);
			}

		}
		else if(pid < 0){
			perror("fork");
			return;
		}

        // parent process
        if(!_background){
            if(DEBUG){
               printf("waiting for child\n");
            }

			// wait for SIGCHLD handler to reap the child
			while(!children_reaped);
			children_reaped = false;

            if(DEBUG){
               printf("child done\n\n\n");
            }
        }
	}

    // make sure all i/o are back to default
    dup2( defaultin, 0 );
    dup2( defaultout, 1 );
    dup2( defaulterr, 2 );

    // close all
	close(fdpipe[1]);
	close(fdpipe[0]);
	close(defaultin);
	close(defaultout);
	close(defaulterr);

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("\nTofi Terminal>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);


int 
main()
{
	// ignore control C
	signal(SIGINT, SIG_IGN);

	FILE *logfile = fopen(LOGFILE, "a+");
    if (logfile == NULL) {
        perror("Error opening log file");
        return 1;
    }
	fprintf(logfile, "\nShell session started:\n");
    fclose(logfile);

	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

