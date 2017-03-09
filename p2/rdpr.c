#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#include "helpers.h"

// TCP Server
/*
http://stackoverflow.com/questions/12730477/close-is-not-closing-socket-properly
http://stackoverflow.com/questions/8872988/properly-close-a-tcp-socket?rq=1
http://www.linuxhowtos.org/C_C++/socket.htm
*/
int main(int argc, char **argv)
{
   if(argc != 4){
		printf("need program, receiver ip, receiver port, and filename, incorrect amount of arguments");
		return 0;
	}
    
    char* host_ip = argv[1];
    int host_port = atoi(argv[2]);
    char* sender_ip;
    int sender_port;
    char* filename = argv[3];
    
    FILE* filecontent = fopen(filename, "rb");
    
    if(filecontent == NULL)
   {
        perror("Could not find provided file...\n");
		exit(EXIT_FAILURE);
   }
    //initialize socket data
    //set up host
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	
    if(sock < 0)
    {
        perror("Could not open socket..\n");
		exit(EXIT_FAILURE);
    }
    
	struct sockaddr_in sa_host;

	memset(&sa_host,0, sizeof sa_host);
	sa_host.sin_family = AF_INET;
	sa_host.sin_addr.s_addr = inet_addr(host_ip);
	sa_host.sin_port = host_port;
	
	socklen_t flen = sizeof(sa_host);
	ssize_t rsize;
	char buffer[MAX_WINDOW_IN_PACKETS][MAX_PACKET_SIZE];
   
   //sock opt and bind
   int sockopt = 1;
	if(-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))){
		perror("Sock options failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	

	if(-1 == bind(sock, (struct sockaddr *)&sa_host, sizeof sa_host)){
		perror("binding failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
   
   
   int select_result;
   fd_set read_fds;
   FD_ZERO(&read_fds);
   FD_SET(sock, &read_fds);
   
   int bufferAvail = 0;
    while(1)
    {
        select_result = select(sock + 1,&read_fds, 0, 0, 0);
        if(select_result == -1)
		{
			perror("select call failed closing socket and exiting application...\n");
			close(sock);
			exit(EXIT_FAILURE);
		}
		if(FD_ISSET(sock, &read_fds))
		{
			rsize = recvfrom(sock, (void*)buffer[bufferAvail], sizeof(buffer[bufferAvail]), 0, (struct sockaddr*)&sa_host, &flen);
			printf(buffer[bufferAvail]);
		    
		}
		
		//reset select
		FD_ZERO( &read_fds );
		FD_SET( sock, &read_fds);
    }
 
   //do stuff
   return 1;
}
