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
    char* buffer = (char*)calloc(MAX_PACKET_SIZE+1, sizeof(char));
    
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
    
    //socklen_t flen_host;
    socklen_t flen_peer;
    
    //flen_host = sizeof(sa_host);
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
    statistics.rst = 0;
    statistics.syn = 0;
    statistics.fin = 0;
    statistics.ack = 0;
    statistics.rst_r = 0;
    //sent /received stats data/bytes
    statistics.data_total = 0;
    statistics.data_unique = 0;
    statistics.packet_total = 0;
    statistics.packet_unique = 0;

    enum system_states sys_state = SYNACK;
    
    gettimeofday(&statistics.start, NULL);

    int sys_seq = 0;
    packet* pack = NULL;
    int last_indx_sent_not_acked = -1;
    packet** sent_packets_not_acked = {NULL};

    struct timeval timeout;
    
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;


    int select_result;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    //for random seq number
    srand(time(NULL)); 

    statistics.syn++;
    sys_seq = SYN_send(sock, &sa_host, &sa_peer, flen_peer);
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
            sys_seq = SYN_send(sock, &sa_host, &sa_peer, flen_peer);
            //reset timer
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
        }
        if(FD_ISSET(sock, &read_fds))
	    {
            int bytes = recvfrom(sock, buffer, MAX_PACKET_SIZE+1, 0, (struct sockaddr*) &sa_peer, &flen_peer);
            if(bytes <= 0){
                perror("Issue receiving from receiver\n");
		        close(sock);
		        exit(EXIT_FAILURE);
            }
            else
            {
                sys_state = DATA;
                pack = packet_parse(buffer);
                statistics.ack++;
                //bulk send changes the value of the array size save it before
                int indx_before = (last_indx_sent_not_acked == -1?0: last_indx_sent_not_acked);
                int i=0;

                packet ** received = bulksendDAT(sock, &sa_host, &sa_peer, flen_peer, filecontent, &sys_seq, &sys_state, pack, &last_indx_sent_not_acked);
                sent_packets_not_acked =(packet **) realloc(sent_packets_not_acked, (last_indx_sent_not_acked+1) * sizeof(packet*));

                //iterate over both and add* to array
                for(;indx_before<last_indx_sent_not_acked; indx_before++ ){
                    sent_packets_not_acked[indx_before] = received[i];
                    i++;
                }

                free(pack);
                timeout.tv_sec = 2;
                timeout.tv_usec = 0;
		break;
            }
        }
        
    }
    int ack_count = 0;
    int last_ack = 0;
    char logType;
    
    while(1)
    {
        select_result = select(sock + 1,&read_fds, 0, 0, &timeout);
        logType = 0;;
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
            logType = 'S';
		    
		    //retramsmit not yet acknowleged segment with last_ack ackno
		    //start timer
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
	    }
	    if(FD_ISSET(sock, &read_fds))
	    {
            memset(buffer, '\0', MAX_PACKET_SIZE+1);
	        rsize = recvfrom(sock, (void*)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&sa_peer, &flen_peer);
		    if(rsize < 0)
		    {
			    perror("receive from failed closing socket and exiting application...\n");
			    close(sock);
			    exit(EXIT_FAILURE);
		    }
            else{
                pack = packet_parse(buffer);
                if(last_ack == pack->ack){
                    ack_count += 1;
                    logType = 'R';
                    if (ack_count == 3){
                        log_packet(logType, &sa_host, &sa_peer, pack);
                        logType = 'S';
                    }
                }
                else{
                    ack_count = 0;
                    logType = 'r';
                }
                last_ack = pack->ack;
            }
            if(logType){
                log_packet(logType, &sa_host, &sa_peer, pack);
            }

        switch(pack->type){
            case ACK:
                statistics.ack++;
                if(logType == 'S'){ //resend packet with last_ack = ack + seqno
                    int i = 0;
                    packet packet_dat;
                    while(i < last_indx_sent_not_acked){
                        if(sent_packets_not_acked[i]->seq > last_ack && sent_packets_not_acked[i]->seq <= (last_ack + MAX_PACKET_SIZE)){
                            //found the packet to send
                            packet_dat = *sent_packets_not_acked[i];
                            break;
                        }
                        i++;
                    }
                    char* dat_str = packet_to_string(&packet_dat);

                    sendto(sock, dat_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) &sa_peer, flen_peer);
                    log_packet('s', &sa_host, &sa_peer, &packet_dat);
                }
                else if(logType == 'r' || logType == 'R'){// normal process
                    int i=0;
                    while(i< last_indx_sent_not_acked){
			if(sent_packets_not_acked[i] == 0){
			    last_indx_sent_not_acked = i;
			}
                        else if(sent_packets_not_acked[i]->seq == last_ack){
                            //remove this packet by replacing by last element
                            sent_packets_not_acked[i] = sent_packets_not_acked[last_indx_sent_not_acked];
                            sent_packets_not_acked[last_indx_sent_not_acked] = 0;

                            //decrement counter and realloc list 
                            last_indx_sent_not_acked--;
                            sent_packets_not_acked = (packet **) realloc(sent_packets_not_acked, (last_indx_sent_not_acked+1) * sizeof(packet*));
                            break;
                        }
                        i++;
                    }

                    int indx_before = last_indx_sent_not_acked;
                    i=0;

                    packet ** received = bulksendDAT(sock, &sa_host, &sa_peer, flen_peer, filecontent, &sys_seq, &sys_state, pack, &last_indx_sent_not_acked);
                    sent_packets_not_acked =(packet **) realloc(sent_packets_not_acked, (last_indx_sent_not_acked+1) * sizeof(packet*));

                    //iterate over both and add* to array
                    for(;indx_before<last_indx_sent_not_acked; indx_before++ ){
                        sent_packets_not_acked[indx_before] = received[i];
                        i++;
                    }

                }
                break;
            case FIN:
		statistics.fin++;
                log_stats(&statistics, 1);
                exit(0);
                break;
            case RST:
                sys_state = RESET;
                exit(-1);
                break;
            default:

                printf("sender received invalid type");
                break;
            }
		
	    }
	
	    //reset select
	    FD_ZERO( &read_fds );
	    FD_SET( sock, &read_fds);
    }

    // do stuff
    return 1;
}

