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
#define MAX_PACKET_SIZE 1024 
#define TIMEOUT 600  
#define MAX_PAYLOAD_SIZE 900 

enum packet_type{DAT,ACK,SYN,FIN,RST};

//packet with next packet, type, seq, ack, payload, window, data, timeout
typedef struct packet{
    struct packet* link;

    enum packet_Type type;
    int payload;
    int win;
    int seq;
    int ack;
    char* data;

    struct timeval timeout;

} packet;

typedef stats{
    //sent /received stats packets
    int syn;
    int fin;
    int rst;
    int ack;
    //sent /received stats data/bytes
    int data_total;
    int data_unique;
    int packet_total;
    int packet_unique;
    //start time
    struct timeval start; 
} stats;

char* getFileContent(char* dir);
char ** separateString(char *buffer, const char *separator);
int sendMsg(int sock,char *sentstr,struct sockaddr* saptr, socklen_t flen);
packet* packet_parse(char* buffer);

char* packet_to_string(packet*);
packet* packet_to_window(packet* packet, packet* head, char* file, int* window_size);
void bulksendDAT(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len,
                       char* file, int* current_seqno, packet* last_ack, packet* timeout_queue);

void ACK(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int win);
void SYN(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len);
void FIN(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, packet* pack);
void DAT(int sock, struct sockaddr_in* self_address, struct sockaddr_in* partner_sa, socklen_t partner_sa_len, int seq, int increment, packet* pack);

void log_stats(stats* statistics, int is_sender);
void log_packet(char event_type, struct sockaddr_in* source, struct sockaddr_in* destination, packet* pack);

packet* remove_from_timers(packet* packet, packet* queue);
packet* add_to_timers(packet* packet, packet* queue);

