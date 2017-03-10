#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include "helpers.h"

int main(int argc, char **argv)
{
   if(argc != 6){
		printf("need program, sender ip, sender port, receiver ip, receiver port and filename, incorrect amount of arguments");
		return 0;
	}
    ssize_t rsize;
    char buffer[1024];
    
    char* host_ip = argv[1];
    int host_port = atoi(argv[2]);
    char* receiver_ip = argv[3];
    int receiver_port = atoi(argv[4]);
    char* filename = argv[5];
    
    FILE* filecontent = fopen(filename, "rb");
   
   if(filecontent == NULL)
   {
        perror("Could not find provided file...\n");
		exit(EXIT_FAILURE);
    }
    

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(sock < 0)
    {
        perror("Could not open socket..\n");
		exit(EXIT_FAILURE);
    }
    struct sockaddr_in sa_host;
    struct sockaddr_in sa_peer;
    
    sa_host.sin_family = AF_INET;
    sa_host.sin_port = host_port;
    sa_host.sin_addr.s_addr = inet_addr(host_ip);
    
    sa_peer.sin_family = AF_INET;
    sa_peer.sin_port = receiver_port;
    sa_peer.sin_addr.s_addr = inet_addr(receiver_ip);
    
    socklen_t flen_host;
    socklen_t flen_peer;
    
    flen_host = sizeof(sa_host);
    flen_peer = sizeof(sa_peer);
    
    int sockopt = 1;

     //sock opt and bind
	if(-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt))){
		perror("Sock options failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	

	if(-1 == bind(sock, (struct sockaddr *)&sa_host, sizeof sa_host)){
		perror("binding failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
    stats statistics;
	//non block
	fcntl(sock, F_SETFL, O_NONBLOCK);
    
    gettimeofday(&statistics.start_time, NULL);
    
	//int Ack = 0;
	int send_next = 0;
    int ack = 0;
    int remaining_space = NULL; //no data
   
    struct timeval timeout;
    
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int select_result;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    long unsigned int position_of_file = 0;
   
    
    while(1)
    {
        select_result = select(sock + 1,&read_fds, 0, 0, &timeout);
        if(select_result == -1)
	    {
		    perror("select call failed closing socket and exiting application...\n");\
		    close(sock);
		    exit(EXIT_FAILURE);
	    }
	    if(select_result == 0)
	    {
		    //timeout
		    
		    //retramsmit not yet acknowleged segment with smallest sequence number
		    //start timer
		     printf("timeout closing application\n");
			 close(sock);
			 return 1;
	    }
	    if(FD_ISSET(sock, &read_fds))
	    {
	        rsize = recvfrom(sock, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&sa_peer, &flen_peer);
		    if(rsize < 0)
		    {
			    perror("receive from failed closing socket and exiting application...\n");
			    close(sock);
			    exit(EXIT_FAILURE);
		    }
		
	    }
	
	    //reset select
	    FD_ZERO( &read_fds );
	    FD_SET( sock, &read_fds);
    }

    // do stuff
    return 1;
}
