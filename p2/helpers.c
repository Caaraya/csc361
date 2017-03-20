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

const char* pkt_type_arr[] = { "DAT", "ACK", "SYN", "FIN", "RST" };

int sendMsg(int sock,char *sentstr,struct sockaddr* saptr, socklen_t flen)
{
	//send request(do we need to handle large files?)
	if (sendto(sock,sentstr,strlen(sentstr),0, saptr, flen)==-1) 
	{
		//send failed
        return 0;
	}
    return 1;
}

//use string tokenizer to split string by delimiter and return string array
char ** separateString(char *buffer, const char *separator, int* len)
{
    char **result = NULL;
    int count = 0;
    char *pch;

    pch = strtok (buffer,separator);

    while (pch != NULL)
    {
        if(strcmp(pch, "\n") == 0){
            break;
        }
        result = (char**)realloc(result, sizeof(char*)*(count+1));
        if (result == NULL)
            exit (-1); 
        result[count] = (char*)malloc(strlen(pch)+1);
        strcpy(result[count], pch);
        count++;
        pch = strtok (NULL, separator);
    }
    if(strcmp(pch, "\n") == 0){
        result = (char**)realloc(result, sizeof(char*)*(count+1));
        if (result == NULL)
            exit (-1); 
        result[count] = (char*)malloc(strlen(pch)+1);
        strcpy(result[count], pch);
        count++;
        pch = strtok (NULL, "");
        if(pch != NULL){
            result = (char**)realloc(result, sizeof(char*)*(count+1));
            if (result == NULL)
                exit (-1); 
            result[count] = (char*)malloc(strlen(pch)+1);
            strcpy(result[count], pch);
            count++;
        }
        else{
            result = (char**)realloc(result, sizeof(char*)*(count+1));
            if (result == NULL)
                exit (-1); 
            result[count] = (char*)malloc(strlen("")+1);
            strcpy(result[count], "");
            count++; 
        }
    }
    *len = count;
    return result;
}

packet packet_parse(char* buffer){
    packet res;// = (packet*)calloc(1, sizeof(struct packet));
    char **result = NULL;
    int len;
    result = separateString(buffer, " ", &len);
    int i;
    for( i=0; i < len; i++) { //result[i-1] != "\n"; i++){
        if(strcmp(result[i], "_magic_") == 0){
            if(!strcmp(result[i+1], "CSC361")){
                //good
            }
            else{
                //bad
            }
        }
        else if(!strcmp(result[i], "_type_")){
            if(!strcmp(result[i+1], "SYN")){
                i+=1;
                res.type = SYN;
            }
            if(!strcmp(result[i+1], "ACK")){
                i+=1;
                res.type = ACK;
            }
            else if(!strcmp(result[i+1], "DAT")){
                i+=1;
                res.type = DAT;
            }
            else if(!strcmp(result[i+1], "FIN")){
                i+=1;
                res.type = FIN;
            }else if(!strcmp(result[i+1], "RST")){
                i+=1;
                res.type = RST;
            }
        }
        else if(!strcmp(result[i], "_seqno_")){
            i+=1;
            res.seq = atoi(result[i]);
        }
        else if(!strcmp(result[i], "_ackno_")){
            i+=1;
            res.ack = atoi(result[i]);
        }
        else if(!strcmp(result[i], "_length_")){
            i+=1;
            res.payload = atoi(result[i]);
        }
        else if(!strcmp(result[i], "_size_")){
            i+=1;
            res.win = atoi(result[i]);
        }
        else if(!strcmp(result[i], "\n")){
            i+=1;
            if(result[i] == NULL){
                res.data = (char*)calloc(1, sizeof(char));
                strcpy(res.data, "");//)res->data = "";
            }
            else{
                res.data = (char*)malloc(strlen(result[i])+1);
                //res.data = result[i];
		strcpy(res.data, result[i]);
            }
            /*strcpy(data, "");
            while(i < len){
                strcat(data, result[i]);
                if(!strcmp(result[i], "\0")){
                    strcat(data, " ");
                }
		else{
		    break;
		}
                i++;
            }*/
	    //res->data= data;
            break;
        }
    }
    for( i=0; i < len; i++) {
        if(result[i]){
            free(result[i]);
        }
    }
    free(result);
    return res;
}

char* packet_to_string(const packet* const source){
    char* data = (char*)calloc(MAX_PACKET_SIZE+1, sizeof(char));
    char istr[10];
    sprintf(data, "_magic_ CSC361 _type_ %s", pkt_type_arr[source->type]);
    if(source->seq){
        strcat(data, " _seqno_ ");
        sprintf(istr, "%d", source->seq);
        strcat(data, istr);
    }
    if(source->ack){
        strcat(data, " _ackno_ ");
        sprintf(istr, "%d", source->ack);
        strcat(data, istr);
    }
    if(source->payload){
        strcat(data, " _length_ ");
        sprintf(istr, "%d", source->payload);
        strcat(data, istr);
    }
    if(source->win){
        strcat(data, " _size_ ");
        sprintf(istr, "%d", source->win);
        strcat(data, istr);
    }
    if(source->data){
        strcat(data, " \n ");
	strcat(data, source->data);
    }

    return data;
    
}

void packet_copy(struct packet* const dest, const struct packet* const src)
{
    assert(src != NULL);
    //assert(src->data != NULL);
    dest->type = src->type;
    dest->payload = src->payload;
    dest->win = src->win;
    dest->seq = src->seq;
    dest->ack = src->ack;
    if(src->data != NULL){
        dest->data = (char*)malloc(MAX_PAYLOAD_SIZE);
        strcpy(dest->data, src->data);
    }
    else{
        dest->data = NULL;
    }
}
void packet_destruct(struct packet* const destructed_packet)
{
    assert(destructed_packet != NULL);
    if(destructed_packet->data != NULL)
        free(destructed_packet->data); 
}

int packetnotNull(struct packet* pck){ //returns 0 if null and 1 if not
    if(pck == NULL || (pck->seq == 0 && pck->ack == 0 && pck->data == 0
     && pck->payload ==0 && pck->win == 0)){
        return 0;
    }
    return 1;
}

//for server
struct packet process_packets(struct packet* const pack, packet* window_arr, FILE* file, int* window_size, int* acked_to){
    
    packet empty;// = {0};
    memset(&empty, 0, sizeof(empty));
    empty.data = NULL;
    packet copy[MAX_WINDOW_IN_PACKETS];
    //memcpy(copy, window_arr, sizeof(packet)*MAX_WINDOW_IN_PACKETS);
    int ii = 0;
    for(; ii < MAX_WINDOW_IN_PACKETS; ii++)
    {
       packet_copy(&copy[ii], &window_arr[ii]);
       packet_destruct(&window_arr[ii]);
    } 
    
    packet current_pack; //= copy[0];
    
    int last_index = -1;
    if(!packetnotNull(&copy[0])){
	assert(pack != NULL);
        packet_copy(&copy[0], pack);
        packet_copy(&current_pack, pack);
        last_index = 0;
    }
    else{
        packet_copy(&current_pack, &copy[0]);
        //get expected index of new packet
        int indx_pack = ((pack->seq - current_pack.seq)/MAX_PAYLOAD_SIZE + ((pack->seq - current_pack.seq)%MAX_PAYLOAD_SIZE != 0));//should be unique
        int indx = 0;
        int potentialLoss = 0;
        if(!packetnotNull(&copy[indx_pack])){
            packet_copy(&copy[indx_pack], pack);
        }
        else{
            //duplicate? hopefully never happens ignore for now
        }
        while(indx < MAX_WINDOW_IN_PACKETS)
        {
            int next_seq = current_pack.seq + current_pack.payload;
            if(packetnotNull(&copy[indx])){
                if(copy[indx].seq == next_seq){
                    //we found a valid packet
                    packet_copy(&current_pack, &copy[indx]);
                    last_index = indx;
                }
            }
            else{
                int window = 0;
                while(indx < MAX_WINDOW_IN_PACKETS)
                {
                    if(packetnotNull(&copy[indx])){
                        potentialLoss = 1;
                    }
                    else{
                        window++;
                    }
                     indx++;
                }
                *window_size = window;
                break; // break when null hit last indes is last null
            }
            indx++;
        }

        if(!potentialLoss &&(current_pack.payload < MAX_PAYLOAD_SIZE || last_index == MAX_WINDOW_IN_PACKETS-1)){//if last dat or filed list or 
            //start processing
            int finalPass = 0;
            while(finalPass < last_index)
            {
                fwrite(copy[finalPass].data, sizeof(char), copy[finalPass].payload, file);
                *acked_to = copy[finalPass].seq;
                packet_copy(&current_pack, &copy[finalPass]);
                packet_copy(&copy[finalPass], &empty);
                finalPass++;
            }
            *window_size = MAX_WINDOW_IN_PACKETS;
        }

    }
    ii = 0;
    //packet_copy(pack, &current_pack);
    
    for(; ii < MAX_WINDOW_IN_PACKETS; ii++)
    {
       packet_copy(&window_arr[ii], &copy[ii]);
       packet_destruct(&copy[ii]);
    } 
    //memcpy(window_arr, copy, sizeof(packet)*MAX_WINDOW_IN_PACKETS);
    packet_destruct(pack);
    return current_pack;
}
//for client
packet** bulksendDAT(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, FILE* file,  int* current_seqno, enum system_states *stat, packet* last_received, int* last_indx){
    //get remaining window availability
    int send_packets = (last_received->win < 5*MAX_PACKET_SIZE?(int)((last_received->win)/MAX_PACKET_SIZE + (last_received->win)%MAX_PACKET_SIZE): 4);

    //save packets in array
    //packet* window_arr[send_packets + 1] = calloc((send_packets+1), sizeof(struct packet));
    packet** window_arr = (packet**)calloc((send_packets+1), sizeof(struct packet*));
    
    int indx = 0;
    while(send_packets > 0)
    {
        packet* pack = (packet*)calloc(1, sizeof(struct packet));
        pack->data = (char*)calloc(MAX_PAYLOAD_SIZE+1, sizeof(char));

        int packets_read;//payload

        if((packets_read = fread(pack->data, sizeof(char), MAX_PAYLOAD_SIZE, file)) > 0){
            window_arr[indx] = pack;
            DAT_send(sock, self_address, partner_sa, partner_sa_len, *current_seqno, packets_read, pack);
            *current_seqno += packets_read;
        }
        else{//fin
            FIN_send(sock, self_address, partner_sa, partner_sa_len, *current_seqno, pack);
            *stat = EXIT;
            break;
        }
        send_packets--;
        indx++;
    }
    if(*last_indx == -1){
        *last_indx = indx;
    }
    else{
        *last_indx += indx;
    }
    return window_arr;
}

void ACK_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int win){ 
    packet ack_pack;
    ack_pack.type = ACK;
    ack_pack.seq = 0;
    ack_pack.ack = seq;
    ack_pack.payload = 0;
    ack_pack.win = win;
    ack_pack.data = (char*)calloc(1, sizeof(char));
    strcpy(ack_pack.data, "");
    char* ack_str = packet_to_string(&ack_pack);

    sendto(sock, ack_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) partner_sa, partner_sa_len);
    log_packet('s', self_address, partner_sa, &ack_pack);

    free(ack_str);
    free(ack_pack.data);
} 
int SYN_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len){
    int seqno = rand();

    packet syn_pack;
    syn_pack.type = SYN;
    syn_pack.seq = seqno;
    syn_pack.ack = 0;
    syn_pack.payload = 0;
    syn_pack.win = 0;
    syn_pack.data = (char*)calloc(1, sizeof(char));
    strcpy(syn_pack.data, "");
    char* syn_str = packet_to_string(&syn_pack);

    sendto(sock, syn_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) partner_sa, partner_sa_len);
    log_packet('s', self_address, partner_sa, &syn_pack);

    free(syn_str);
    free(syn_pack.data);
    return seqno;
}
void FIN_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, packet *fin_pack){
    fin_pack->type = FIN;
    fin_pack->seq = seq;
    fin_pack->ack = 0;
    fin_pack->payload = 0;
    fin_pack->win = 0;
    fin_pack->data = (char*)calloc(1, sizeof(char));
    strcpy(fin_pack->data, "");
    char* fin_str = packet_to_string(fin_pack);

    sendto(sock, fin_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) partner_sa, partner_sa_len);
    log_packet('s', self_address, partner_sa, fin_pack);

    free(fin_str);
    free(fin_pack->data);
}
void DAT_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int increment, packet *dat_pack){
    dat_pack->type = DAT;
    dat_pack->seq = seq;
    dat_pack->ack = 0;
    dat_pack->payload = increment;
    dat_pack->win = 0;
    char* dat_str = packet_to_string(dat_pack);

    sendto(sock, dat_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) partner_sa, partner_sa_len);
    log_packet('s', self_address, partner_sa, dat_pack);
}
void RST_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int window_size){
    packet rst_pack;
    rst_pack.type = RST;
    rst_pack.seq = 0;
    rst_pack.ack = seq;
    rst_pack.payload = 0;
    rst_pack.win = window_size;
    rst_pack.data = (char*)calloc(1, sizeof(char));
    strcpy(rst_pack.data, "");
    char* rst_str = packet_to_string(&rst_pack);

    sendto(sock, rst_str, MAX_PACKET_SIZE, 0, (struct sockaddr*) partner_sa, partner_sa_len);
    log_packet('s', self_address, partner_sa, &rst_pack);

    free(rst_str);
    free(rst_pack.data);
}

void log_stats(stats* stat, int is_sender){

    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval diff;
    timersub(&now, &stat->start, &diff);


    if(is_sender){
        //no easier way found, fprintf to the rescue
         fprintf(stdout, "total data bytes sent: %d\n"
                    "unique data bytes sent: %d\n"
                    "total data packets sent: %d\n"
                    "unique data packets sent: %d\n"
                    "SYN packets sent: %d\n"
                    "FIN packets sent: %d\n"
                    "RST packets sent: %d\n"
                    "ACK packets sent: %d\n"
                    "RST packets recieved: %d\n"
                    "total time duration (second): %d.%d\n",
                     stat->data_total,
                     stat->data_unique,
                     stat->packet_total,
                     stat->packet_unique,
                     stat->syn,
                     stat->fin,
                     stat->rst,
                     stat->ack,
                     stat->rst_r,
                     (int) diff.tv_sec,
                     (int) diff.tv_usec);
    }
    else{
        fprintf(stdout, "total data bytes recieved: %d\n"
                    "unique data bytes recieved: %d\n"
                    "total data packets recieved: %d\n"
                    "unique data packets recieved: %d\n"
                    "SYN packets recieved: %d\n"
                    "FIN packets recieved: %d\n"
                    "RST packets recieved: %d\n"
                    "ACK packets sent: %d\n"
                    "RST packets sent: %d\n"
                    "total time duration (second): %d.%d\n",
                     stat->data_total,
                     stat->data_unique,
                     stat->packet_total,
                     stat->packet_unique,
                     stat->syn,
                     stat->fin,
                     stat->rst,
                     stat->ack,
                     stat->rst_r,
                     (int) diff.tv_sec,
                     (int) diff.tv_usec);
    }
}
void log_packet(char event_type, struct sockaddr_in* source, struct sockaddr_in* destination, struct packet* const pack){
    char strtime[100];
    struct timeval tv;
    time_t now;
    struct tm* nowtm;
    //get time
    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    nowtm = localtime(&now);
    strftime(strtime, 100, "%H:%M:%S", nowtm);
    //fprintf to the rescue
    fprintf(stdout, "%s.%0li %c %d:%s %d:%s %s %d/%d %d/%d\n",
        strtime, (long int) tv.tv_usec, event_type, source->sin_port, inet_ntoa(source->sin_addr), destination->sin_port, 
        inet_ntoa(destination->sin_addr), pkt_type_arr[pack->type], pack->seq, pack->ack, pack->payload, pack->win);
}

