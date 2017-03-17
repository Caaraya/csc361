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

#define MAX_WINDOW_IN_PACKETS 10
#define MAX_PACKET_SIZE 900 
#define TIMEOUT 600  
#define MAX_PAYLOAD_SIZE 1024 

enum system_states {SYNACK,DATA,RESET,EXIT};

enum packet_type{DAT,ACK,SYN,FIN,RST};
// one timer if times out stop, if ack received reset, if three of the same ack sent, packet lost
//packet with next packet, type, seq, ack, payload, window, data, timeout
typedef struct packet {

    enum packet_type type;
    int payload;
    int win;
    int seq;
    int ack;
    char* data;

} packet;

typedef struct stats {
    //sent /received stats packets
    int syn;
    int fin;
    int rst;
    int ack;
    int rst_r;
    //sent /received stats data/bytes
    int data_total;
    int data_unique;
    int packet_total;
    int packet_unique;
    //start time
    struct timeval start; 
} stats;

int packetnotNull(struct packet* pck);
char ** separateString(char *buffer, const char *separator, int* len);
int sendMsg(int sock,char *sentstr,struct sockaddr* saptr, socklen_t flen);
struct packet* packet_parse(char* buffer);

char* packet_to_string(packet*);
void process_packets(struct packet* pack, struct packet* window_arr, FILE* file, int* window_size, int* acked_to);
packet** bulksendDAT(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, FILE* file, int* current_seqno, enum system_states  *stat, packet* last_received, int* indx);

void ACK_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int win);
int SYN_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len);
void FIN_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, packet* pack);
void DAT_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int increment, packet* pack);
void RST_send(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int window_size);

void log_stats(stats* statistics, int is_sender);
void log_packet(char event_type, struct sockaddr_in* source, struct sockaddr_in* destination, struct packet* pack);

