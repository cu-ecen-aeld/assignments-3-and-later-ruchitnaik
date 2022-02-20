/**
 *  @file	aesdsocket.c
 *  @author	Ruchit Naik
 *  @date	02/13/2022
 *	@brief	The file contains code to create IPv4 socket over port 9000. It receives string
 *			and write to a temporary file. The socket then reads the file and sends it back
 *			to the client. The socket handles SIGINT and SIGTERM to terminate the process
 *			smoothly and delete the temporary file used to write the string. The code also
 *			daemonizes the socket to run in background.
 *	@ref	https://www.delftstack.com/howto/c/getaddrinfo-in-c/
 *	        https://beej.us/guide/bgnet/html/#a-simple-stream-server
**/

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT				"9000"
#define MAX_CONNECTIONS		10								//Number of pending requests being queued
#define MAXDATALEN			1024							//Maximum permissible data to be received over socket
#define FILE				"/var/tmp/aesdsocketdata"		//File path where the received data is to be received

static int fd_socket, fd_client, fd;
static struct addrinfo *res;								//Pointer to the structure to get struct sockaddr

static void sig_handler(int signo){
	if((signo == SIGINT) || (signo == SIGTERM)){
		if(fd_socket>0){
			close(fd_socket);
		}
		if(fd_client>0){
			close(fd_client);
		}
		if(fd>0){
			close(fd);
		}
		//deleting the file
		int ret = remove(FILE);
		if(ret == -1){
			syslog(LOG_ERR, "Error removing the file");
			closelog();
			exit(-1);
		}
		syslog(LOG_DEBUG, "Caught signal,exiting");
		closelog();
	}
	exit(0);
}

/**
 *	@brief	Daemonize the application to run the socket in background
 *	@param	none
 *	@return	none 
 */
void daemonize(void){
	pid_t pid;

	//Ignore the working signal
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	//Create new process
	pid = fork();

	//Error occured
	if(pid < 0){
		exit(EXIT_FAILURE);
	}

	//Success: Parent allowed to terminate
	if(pid > 0){
		exit(EXIT_SUCCESS);
	}

	//Create new session and process group
	if(setsid() == -1){
		exit(-1);
	}

	//umask does not require error handling
	umask(0);

	//set the working directory to the root directory
	if(chdir("/") == -1){
		exit(-1);
	}

	//redirect fd's 0,1,2 to /dev/null
	open("/dev/null", O_RDWR);						
	dup(0);											//stdin
	dup(1);											//stdout
	dup(2);											//stderror

	//close all file descriptors
	close(0);
	close(1);
	close(2);
}

int main(int argc, char **argv){
	int yes = 1;
	int ret;
	struct addrinfo hints;
	char s[INET_ADDRSTRLEN], buf[MAXDATALEN];

	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	
	memset(buf, 0, sizeof(buf));
	memset(&hints, 0, sizeof(hints));					//Clear the hints datastructure

	//Setup syslog logging for the utility using LOG_USER
	openlog(NULL, 0, LOG_USER);

	//Configure SIGINT
	if(signal(SIGINT, sig_handler) == SIG_ERR){
		syslog(LOG_ERR, "Error: Cannot handle SIGINT");
		closelog();
		return -1;
	}
	//Configure SIGTERM
	if(signal(SIGTERM, sig_handler) == SIG_ERR){
		syslog(LOG_ERR, "Error: Cannot handle SIGINT");
		closelog();
		return -1;
	}
	
	hints.ai_flags = AI_PASSIVE;						//Set the flag to make the socket suitable for bind

	ret = getaddrinfo(NULL, PORT, &hints, &res);		//The returned socket would be suitable for binding and accepting connections
	if(ret != 0){
		syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(ret));
		closelog();
		return -1;
	}
	
	//Creating an socket endpoint for communication using IPv4 by socket streaming
	fd_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(fd_socket == -1){
		syslog(LOG_ERR, "Error opening socket: %d", errno);
		closelog();
		return -1;
	}

	//Configure socket to reuse the address
	ret = setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if(ret == -1){
		syslog(LOG_ERR, "Error setsockopt: %d", errno);
		closelog();
		return -1;
	}

	struct addrinfo *p;
	bool bind_flag = false;
	for(p=res; p!=NULL; p=p->ai_next){
		ret = bind(fd_socket, p->ai_addr, p->ai_addrlen);
		if(ret != 1){
			bind_flag = true;
			break;
		}
	}

	if(!bind_flag){
		syslog(LOG_ERR, "Error bind: %d - %s", errno, strerror(errno));
		closelog();
		close(fd_socket);
		return -1;
	}

	// ret = bind(fd_socket, res->ai_addr, res->ai_addrlen);
	// if(ret == -1){
	// 	syslog(LOG_ERR, "Error bind: %d - %s", errno, strerror(errno));
	// 	closelog();
	// 	close(fd_socket);
	// 	return -1;
	// }

	freeaddrinfo(res); 							// all done with this structure
	
	ret = listen(fd_socket, MAX_CONNECTIONS);
	if(ret == -1){
		syslog(LOG_ERR, "Error listen: %d", errno);
		closelog();
		close(fd_socket);
		return -1;
	}

	//Creating the file to store data over the socket
	fd = creat(FILE, 0644);
	if(fd == -1){
		syslog(LOG_ERR, "Error Creat: %d", errno);
		closelog();
		close(fd_socket);
		return -1;
	}
	close(fd);											//Close file after creating it

	//Daemonize after binded to port 9000
	if((argc == 2) && (strcmp("-d", argv[1]) == 0)){
		daemonize();
	}

	while(1){											//accept loop to listen forever
		int size_recv;
		
		fd_client = accept(fd_socket, (struct sockaddr *)&peer_addr, &peer_addr_len);
		if(fd_client == -1){
			syslog(LOG_ERR, "Error accept: %d - %s", errno, strerror(errno));
			perror("accept");
			closelog();
			close(fd_socket);
			return -1;
		}

		//Review from here - Delete when reviewed
		inet_ntop(peer_addr.sin_family, &peer_addr.sin_addr, s, sizeof(s));
		syslog(LOG_DEBUG, "Accepted connection from %s", s);

		//Open file to append the received data
		fd = open(FILE, O_WRONLY);
		lseek(fd, 0, SEEK_END);
		//Loop to receive data form the client and append it to the given file
		while(1){
			//Receive data over socket
			size_recv = recv(fd_client, buf, sizeof(buf), 0);
			if(size_recv == -1){
				syslog(LOG_ERR, "Error recv: %d", errno);
				closelog();
				close(fd_socket);
				close(fd_client);
				return -1;
			}
			write(fd, buf, size_recv);
			if(buf[size_recv-1] == '\n'){
				break;									//Break the while loop if 'EOL received', indicating end of packet
			}
		}
		close(fd);										//Close file once received data is completelty transfered
		memset(buf, 0, sizeof(buf));					//Clear buffer to read in the same buffer

		fd = open(FILE, O_RDONLY);						//Open file to read the data (Read only mode)
		//Loop to read all the data from the file and send it over the socket to the client
		while((ret = read(fd, buf, MAXDATALEN))){
			if(ret == -1){
				if(errno == EINTR){
					continue;
				}
				syslog(LOG_ERR, "Error read: %d", errno);
				closelog();
				close(fd_socket);
				close(fd_client);
				return -1;
			}
			send(fd_client, buf, ret, 0);
		}
		close(fd);										//Close file once completely read
		close(fd_client);								//Close client connection
		syslog(LOG_DEBUG, "Closed connection from %s", s);
	}

	//Close all while existing
	closelog();
	close(fd_client);
	close(fd_socket);
	return 0;
}