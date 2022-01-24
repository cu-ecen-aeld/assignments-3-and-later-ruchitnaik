/**
 * @file	writer.c
 * @author	Ruchit Naik
 * @date	01-22-2022
 * @brief	Application writes the specific string entered from the command line
 * 		to the file along with the path entered from the terminal.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

/*********** PRIVATE MACROS *************/
#define REQ_ARG	3
/****************************************/

int main(int argc, char **argv){
	//Setup syslog logging for the utility using LOG_USER
	openlog(NULL, 0, LOG_USER);

	//Check valid number of arguments passed
	if(argc != REQ_ARG){
		syslog(LOG_ERR, "ERROR: Insufficient arguments passed. Enter valid arguments <file path><string>");
		closelog();					//Close log
		return 1;
	}

	char *filep = argv[1];
	char *string = argv[2];

	//Open file considering the file or directory already exits
	FILE *file_open = fopen(filep, "w");
	if(file_open == NULL){
		syslog(LOG_ERR, "Unable to open file \%s", filep);
		closelog();					//Close log
		return 1;
	}

	if(filep[0] == ' '){
		syslog(LOG_ERR, "ERROR: Invalid file path entered.");
		closelog();					//Close log
		fclose(file_open);				//Close file on error
		return 1;
	}

	syslog(LOG_DEBUG, "writing %s to %s", string, filep);

	//Writing the string to the file
	if((fputs(string, file_open)) < 0){			//Error check
		syslog(LOG_ERR, "ERROR: Failed to write the file \%s", filep);
		closelog();					//Close log
		fclose(file_open);				//Close file
		return 1;
	}

	closelog();						//Close log
	fclose(file_open);					//Close file after successfully writing the string
	return 0;
}
