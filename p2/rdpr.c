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
    char* filename = argv[3];
    
    FILE* filecontent = fopen(filename, "w");
    
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
	struct sockaddr_in sender_address;
	socklen_t sender_flen;

	memset(&sa_host,0, sizeof sa_host);
	sa_host.sin_family = AF_INET;
	sa_host.sin_addr.s_addr = inet_addr(host_ip);
	sa_host.sin_port = host_port;
	

	//socklen_t flen = sizeof(sa_host);
	sender_flen = sizeof(sa_host);
	ssize_t rsize;
	//needed for processing
	char* buffer = (char*)calloc(MAX_PACKET_SIZE+1, sizeof(char));
	int acked_to = 0;
	char* packet_str;
	unsigned long first_addr = 0;
   
   //sock opt and bind
   int sockopt = 1;
	if(-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))){
		perror("Sock options failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	

	if(-1 == bind(sock, (struct sockaddr *)&sa_host, sizeof(sa_host))){
		perror("binding failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}

	//initialize stats
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

	gettimeofday(&statistics.start, NULL);
	//window
	struct packet window[MAX_WINDOW_IN_PACKETS] = {{0}};

	int window_size = MAX_WINDOW_IN_PACKETS;
	
   int select_result;
   fd_set read_fds;
   FD_ZERO(&read_fds);
   FD_SET(sock, &read_fds);
   int acked_to_same = 0;
   
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
			memset(buffer, '\0', MAX_PACKET_SIZE);
			packet* pack;
			rsize = recvfrom(sock, (void*)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&sender_address, &sender_flen);
			if(rsize == -1){
				printf("recv error\n");
			}

			//printf(buffer);//
			//printf(first_addr);
			//printf(sender_address.sin_addr.s_addr);

			if(first_addr == 0){
				first_addr = sender_address.sin_addr.s_addr;
			} else if(first_addr != sender_address.sin_addr.s_addr){
				//reset
				RST_send(sock, &sa_host, &sender_address, sender_flen, pack->seq, 0);
				exit(-1);
			}
		    
			pack = packet_parse(buffer);

			if(pack == NULL){
				printf("packet corrupt");
				continue;
			}
			if(pack->seq < acked_to){
				statistics.data_total += pack->payload;
				statistics.packet_total++;
				ACK_send(sock, &sa_host, &sender_address, sender_flen, pack->seq, 0);
				continue;
			}

			char log_type = 'r';
			// before smaller than smallest sequence
			if(packetnotNull(&window[0]) && pack->seq < window[0].seq){
				log_type = 'R';
			}

			log_packet(log_type, &sa_host, &sender_address, pack);
			//change variables based on packet type
			switch(pack->type){
				case SYN:
					//got syn request
					statistics.syn++;
					statistics.ack++;
					if(!packetnotNull(&window[0])){
					    acked_to_same = 0;
					}
					else{
					    acked_to_same = (window[0].seq == acked_to ? 0 : acked_to == pack->seq);
					}
					acked_to = pack->seq;
					ACK_send(sock, &sa_host, &sender_address, sender_flen, pack->seq, (int)( MAX_PAYLOAD_SIZE * window_size));
					log_packet((acked_to_same?'S':'s'), &sa_host, &sender_address, pack);
					break;
				case DAT:{
					statistics.packet_total++;
					if(log_type == 'r') { 
						statistics.packet_unique++;
						statistics.data_unique++;
					}
					statistics.data_total += pack->payload;
					//write to file change acked and packet to last received packet
					int last_acked_to = acked_to;
					process_packets(pack, window, filecontent, &window_size, &acked_to);
					
					if(!packetnotNull(&window[0])){
					    acked_to_same = 0;
					}
					else{
					     acked_to_same = (window[0].seq == acked_to ? 0 : acked_to == last_acked_to);
					}
					//send ack
					statistics.ack++;
					ACK_send(sock, &sa_host, &sender_address, sender_flen, pack->seq, (int)( MAX_PAYLOAD_SIZE * window_size));
					log_packet((acked_to_same?'S':'s'), &sa_host, &sender_address, pack);
					break;
				}break;
				case RST:
					statistics.rst++;
					ACK_send(sock, &sa_host, &sender_address, sender_flen, pack->seq, (int)( MAX_PAYLOAD_SIZE * window_size));
					log_packet('s', &sa_host, &sender_address, pack);
					close(sock);
					exit(-1);
					break;
				case FIN:{
					statistics.fin++;
					//send fin
					int indx = 0;
					while(indx < MAX_WINDOW_IN_PACKETS){
						if (packetnotNull(&window[indx])){
						     fwrite(window[indx].data, sizeof(char), window[indx].payload, filecontent);
						}

						indx++;
					}
					packet_str = packet_to_string(pack);
					sendto(sock, packet_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) &sender_address, sender_flen);
					log_packet('s', &sa_host, &sender_address, pack);
					
					log_stats(&statistics, 0);
					close(sock);
					exit(0);
					break;
				} break;
				default:
					//terrible things
					printf("got an ACK, receiver should never get ACK's\n");
					break;
			}
		}
		//reset select
		FD_ZERO( &read_fds );
		FD_SET( sock, &read_fds);
    }
 
   //do stuff
   return 0;
}

