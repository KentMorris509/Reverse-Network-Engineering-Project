/**
 * client.c
 *
 * @author Kent Morris
 *
 * TODO: In this program I reversed engineered the orginal-client to figure out how to reprogram the exact program in this file
 */

#define _XOPEN_SOURCE 600

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define BUFF_SIZE 1024

long prompt();
int connect_to_host(char *hostname, char *port);
int authANDconnect(int server_fd);
int authANDConnectWeather(int server_fd, char *buff);
void closeGoodby(int server_fd);
void closeSocket(int server_fd);
void airTempature();
void relativeHumidity();
void windSpeed();
void main_loop();
char buff[BUFF_SIZE];
int errorCheck;
int server_fd; 
int junk;
time_t curtime;

int main() {
	main_loop();
	return 0;
}

/**
 * Loop to keep asking user what they want to do and calling the appropriate
 * function to handle the selection.
 */
void main_loop() {
	printf("WELCOME TO THE COMP375 SENSOR NETWORK\n\n");
   	time(&curtime);
	
	while (true) {
		long selection = prompt();

		switch (selection) {
			case 1:
				airTempature();
				break;
			case 2:
				relativeHumidity();
				break;
			case 3:
				windSpeed();
				break;
			case 4:
				printf("GOODBYE!\n");
				close(server_fd);
				exit(0);
				break;
			default:
				fprintf(stderr, "ERROR: Invalid selection\n");
				break;
		}
	}

	close(server_fd);
}


/** 
 * Print command prompt to user and obtain user input.
 *
 * @return The user's desired selection, or -1 if invalid selection.
 */
long prompt() {
	// TODO: add printfs to print out the options
	printf("Which sensor would you like to read:\n\n");
    printf("\t(1) Air tempature\n");
    printf("\t(2) Relative humidity\n");
    printf("\t(3) Wind speed\n");
    printf("\t(4) Quit Program\n\n");
    printf("Selection: ");
	// Read in a value from standard input
	char input[10];
	memset(input, 0, 10); // set all characters in input to '\0' (i.e. nul)
	char *read_str = fgets(input, 10, stdin);

	// Check if EOF or an error, exiting the program in both cases.
	if (read_str == NULL) {
		if (feof(stdin)) {
			exit(0);
		}
		else if (ferror(stdin)) {
			perror("fgets");
			exit(1);
		}
	}

	// get rid of newline, if there is one
	char *new_line = strchr(input, '\n');
	if (new_line != NULL) new_line[0] = '\0';

	// convert string to a long int
	char *end;
	long selection = strtol(input, &end, 10);

	if (end == input || *end != '\0') {
		selection = -1;
	}

	return selection;
}

/*
This function grabs the air tempature from the weatherStation.sandiego.edu server
*/
void airTempature(){
	server_fd = authANDconnect(server_fd);

	memset(buff,0,BUFF_SIZE);
	strcpy(buff,"AIR TEMPERATURE\n");

	errorCheck = send(server_fd, buff, 16, 0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	memset(buff, 0, BUFF_SIZE);

	errorCheck = recv(server_fd,buff, BUFF_SIZE,0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	int air_temperature = 0;
	junk = 0;

	sscanf(buff, "%d %d F",&junk, &air_temperature);
	printf("\nThe last AIR TEMPERATURE reading was %d F, taken at %s\n", air_temperature,ctime(&curtime));

	closeGoodby(server_fd);
}
/*
This function gets the windSpeed from the weatherStation.sandiego.edu server
*/
void windSpeed(){

	server_fd = authANDconnect(server_fd);
	memset(buff,0,BUFF_SIZE);
	strcpy(buff,"WIND SPEED\n");

	errorCheck = send(server_fd, buff, 11, 0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	memset(buff, 0, BUFF_SIZE);

	errorCheck = recv(server_fd,buff, BUFF_SIZE,0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	int wind_speed = 0;
	junk = 0;

	sscanf(buff, "%d %d MPH",&junk, &wind_speed);
	printf("\nThe last WIND SPEED reading was %d MPH, taken at %s\n", wind_speed,ctime(&curtime));

	closeGoodby(server_fd);
}

/*
This function grabs the Relative Humidity from the weatherStation.sandiego.edu server
*/
void relativeHumidity(){
	server_fd = authANDconnect(server_fd);
	memset(buff,0,BUFF_SIZE);
	strcpy(buff,"RELATIVE HUMIDITY\n");

	errorCheck = send(server_fd, buff, 18, 0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	memset(buff, 0, BUFF_SIZE);

	errorCheck = recv(server_fd,buff, BUFF_SIZE,0);
	if (errorCheck == -1){
		fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
		exit(1);
	}

	int relative_humidity = 0;
	junk = 0;

	sscanf(buff, "%d %d %%",&junk, &relative_humidity);
	printf("\nThe last RELATIVE HUMIDITY reading was %d %%, taken at %s\n", relative_humidity,ctime(&curtime));

	closeGoodby(server_fd);
}

/**
 * This function sends close message to end server grabbing for a case
*@param server_fd Server file description byte information
*/
void closeGoodby(int server_fd){
	strcpy(buff,"CLOSE\n");

	send(server_fd, buff, 6, 0);
    
    memset(buff, 0, BUFF_SIZE);

	recv(server_fd,buff, BUFF_SIZE,0);

	closeSocket(server_fd);
}



/**
 * This function Authenticates and Connects to the hopper.sandiego.edu host server for each case call
*@param server_fd Server file description byte information
*@return File descriptor of current socket
*/
int authANDconnect(int server_fd){

	server_fd = connect_to_host("hopper.sandiego.edu", "7030");
	memset(buff,0,BUFF_SIZE);
	strcpy(buff,"AUTH password123\n");

	errorCheck = send(server_fd, buff, 17, 0);
	if (errorCheck == -1){
        fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
        exit(1);
    }

    
    memset(buff, 0, BUFF_SIZE);

	errorCheck = recv(server_fd,buff, BUFF_SIZE,0);
	if (errorCheck == -1){
        fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
        exit(1);
    }
	closeSocket(server_fd);

	server_fd = authANDConnectWeather(server_fd,buff);

	return server_fd;

}

/**
 * This function Authenticates and Connects to the weatherstation.sandiego.edu host server
*@param server_fd Server file description byte information
*@param buff Buffer to store messages to and from servers
*@return File descriptor of current socket
*/
int authANDConnectWeather(int server_fd, char *buff){

	char hostname[256];
	char port[6];
	char authenticator[19];

	memset(hostname, 0, sizeof(hostname));
    memset(port, 0, sizeof(port));
    memset(authenticator, 0, sizeof(authenticator));
	
	sscanf(buff,"CONNECT %s %s %s",hostname, port, authenticator);

	
	server_fd = connect_to_host(hostname,port);
	
	memset(buff,0,BUFF_SIZE);
	strcpy(buff,"AUTH ");
	strcat(authenticator,"\n");
	strcat(buff, authenticator);

	errorCheck = send(server_fd,buff,19,0);
	if (errorCheck == -1){
        fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
        exit(1);
    }

	memset(buff, 0, BUFF_SIZE);

	errorCheck = recv(server_fd,buff,BUFF_SIZE,0);
	if (errorCheck == -1){
        fprintf(stderr, "send error: %s\n", gai_strerror(errorCheck));
        exit(1);
    }

	return server_fd;
}

/**
 * This function shuts down and closes the current socket
*@param socket_fd Socket file description byte information
*/
void closeSocket(int socket_fd) {
    if (shutdown(socket_fd, SHUT_RDWR) == -1) {
        perror("shutdown");
    }

    if (close(socket_fd) == -1) {
        perror("close");
    }
}

/**
 * Socket implementation of connecting to a host at a specific port.
 *
 * @param hostname The name of the host to connect to (e.g. "foo.sandiego.edu")
 * @param port The port number to connect to
 * @return File descriptor of new socket to use.
 */
int connect_to_host(char *hostname, char *port) {
	// Step 1: fill in the address info in preparation for setting 
	//   up the socket
	
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;       // Use IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// get ready to connect
	if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// Step 2: Make a call to socket
	int fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (fd == -1) {
		perror("socket");
		exit(1);
	}

	// Step 3: connect!
	if (connect(fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
		perror("connect");
		exit(1);
	}

	freeaddrinfo(servinfo); // free's the memory allocated by getaddrinfo

	return fd;
}
