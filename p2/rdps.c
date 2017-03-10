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
    char buffer = calloc(MAX_PACKET_SIZE+1, sizeof(char));
    
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

    enum system_states sys_state = SYNACK;
    
    gettimeofday(&statistics.start_time, NULL);

    int sys_seq = 0;
    packet* pack = NULL;
    packet** sent_packets_not_acked;

    struct timeval timeout;
    
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;


    int select_result;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    long unsigned int position_of_file = 0;
    //for random seq number
    srand(time(NULL)); 

    statistics.syn++;
    sys_seq = send_SYN(sock, &sa_peer, flen_peer, &sa_host);
    while(sys_state == SYNACK){
        select_result = select(sock + 1,&read_fds, 0, 0, &timeout);
        if(select_result == -1)
	    {
		    perror("select call failed closing socket and exiting application...\n");\
		    close(sock);
		    exit(EXIT_FAILURE);
	    }
	    if(select_result == 0)
	    {
            statistics.syn++;
            sys_seq = send_SYN(sock, &sa_peer, flen_peer, &sa_host);
            //reset timer
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
        }
        if(FD_ISSET(sock, &read_fds))
	    {
            int bytes = recvfrom(sock, buffer, MAX_PACKET_SIZE+1, 0, (struct sockaddr*) &sa_peer, flen_peer);
            if(bytes <= 0){
                perror("Issue receiving from receiver\n");
		        close(sock);
		        exit(EXIT_FAILURE);
            }
            else
            {
                sys_state = DATA;
                pack = parse_packet(buffer);
                statistics.ack++;
                sent_packets_not_acked = bulksendDAT(sock, sa_host, sa_peer, flen_peer, filecontent, sys_seq, statistics, pack);
                free(pack);
                timeout.tv_sec = 2;
                timeout.tv_usec = 0;
            }
        }
        
    }
    int ack_count;
    packet* last_packet;
    char logType;
    
    while(1)
    {
        select_result = select(sock + 1,&read_fds, 0, 0, &timeout);
        logType = NULL;
        if(select_result == -1)
	    {
		    perror("select call failed closing socket and exiting application...\n");\
		    close(sock);
		    exit(EXIT_FAILURE);
	    }
	    if(select_result == 0)
	    {
		    //timeout
            // not yet implemented
            //logType = 'S'
		    
		    //retramsmit not yet acknowleged segment with smallest sequence number
		    //start timer
		     printf("timeout closing application\n");
			 close(sock);
			 return 1;
	    }
	    if(FD_ISSET(sock, &read_fds))
	    {
            memset(buffer, '\0', MAX_PACKET_LENGTH+1);
	        rsize = recvfrom(sock, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&sa_peer, &flen_peer);
		    if(rsize < 0)
		    {
			    perror("receive from failed closing socket and exiting application...\n");
			    close(sock);
			    exit(EXIT_FAILURE);
		    }
            else{
                pack = parse_packet(buffer);
                logType = 'r';
            }
            if(logType){
                log_packet(logType, &sa_host, &sa_peer, pack);
            }
		
	    }
	
	    //reset select
	    FD_ZERO( &read_fds );
	    FD_SET( sock, &read_fds);
    }

    // do stuff
    return 1;
}
