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

//make separate file with separateString, sendMsg, getFileContent

//use string tokenizer to split string by delimiter and return string array
char ** separateString(char *buffer, const char *separator)
{
    char **result = NULL;
    int count = 0;
    char *pch;

    pch = strtok (buffer,separator);

    while (pch != NULL)
    {
        result = (char**)realloc(result, sizeof(char*)*(count+1));
        if (result == NULL)
            exit (-1); 
        result[count] = (char*)malloc(strlen(pch)+1);
        strcpy(result[count], pch);
        count++;
        pch = strtok (NULL, separator);
    }
    return result;
    }

void sendMsg(int sock,char *sentstr,struct sockaddr* saptr, socklen_t flen)
{
    //send request(do we need to handle large files?)
    printf(sentstr);
    if (sendto(sock,sentstr,strlen(sentstr),0, saptr, flen)==-1) 
    {
	    perror("sending failed closing socket and exiting application...\n");
	    close(sock);
	    exit(EXIT_FAILURE);
    }
}

void generateresponse(int sock, struct sockaddr* saptr, socklen_t flen, int *send_next, int *remaining_space, long unsigned int *position_of_file, char *filecontent)
{
    char buffer[1024];
    char strint[2];
    strint[0] = *send_next+'0';
    strint[1] = '\0';
    strcpy(buffer,"_magic_ CSC361 _type_ ");
    if(remaining_space)
    {
        strcat(buffer, "SYN ");
        strcat(buffer, "_seqno_ ");
        strcat(buffer, strint);
        strcat(buffer, "\n");
        printf(buffer);
        sendMsg(sock, buffer, saptr, flen);
    }
    else if((*position_of_file) == sizeof(*filecontent))
    {
        strcat(buffer, "FIN ");
        strcat(buffer, "_seqno_ ");
        strcat(buffer, strint);
        strcat(buffer, "\n");
        sendMsg(sock, buffer, saptr, flen);
    }
    else
    {
        int index = 0;
        while(index < 3)
        {
            strint[0] = *send_next+'0';
            strcat(buffer, "DAT ");
            strcat(buffer, "_seqno_ ");
            strcat(buffer, strint);
            strcat(buffer, "_length_ ");
            if(*remaining_space > 1024 && (sizeof(filecontent) - (*position_of_file)) >= 900){
                strcat(buffer, "900");
                (*remaining_space) -= 900;
                strcat(buffer, "\n");
                char subbuff[901];
                memcpy(subbuff, &filecontent[(int)position_of_file], 900);
                subbuff[900] = '\0';
                *send_next += 900;
            }   
            else if(*remaining_space > (sizeof(*filecontent) - (*position_of_file))){
                long unsigned int size = (sizeof(filecontent) - (*position_of_file));
                strint[0] = size+'0';
                strcat(buffer, strint);
                (*remaining_space) -= (int)size;
                strcat(buffer, "\n");
                char subbuff[size+1];
                memcpy(subbuff, &filecontent[(int)position_of_file], (int)size);
                *send_next += size;
                subbuff[size] = '\0';
            }
            else
            {
                return;
            }
            sendMsg(sock, buffer, saptr, flen);
            index++;
            memset(buffer, 0, sizeof buffer);
        }
        
    }

}

char* getFileContent(char* dir)
{
    char* filecontent;
    long filesize;
    
    FILE * f = fopen(dir, "rb");
    if(f == NULL)
    {
        return NULL;
    }
    else
    {
        fseek(f, 0, SEEK_END);
        filesize = ftell(f);
        rewind(f);
        filecontent = malloc(filesize * (sizeof(char)));
        fread(filecontent, sizeof(char), filesize, f);
        fclose(f);
        return filecontent;
    }
}

int main(int argc, char **argv)
{
   if(argc != 6){
		printf("need program, sender ip, sender port, receiver ip, receiver port and filename, incorrect amount of arguments");
		return 0;
	}
    ssize_t rsize;
    char buffer[1024];

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in sa;

    memset(&sa,0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &(sa.sin_addr));

    socklen_t flen;
   
   char* filecontent = getFileContent(argv[5]);
   
   if(filecontent == NULL)
   {
        perror("Could not find provided file...\n");
		close(sock);
		exit(EXIT_FAILURE);
    }
    
    flen = sizeof(sa);

     //sock opt and bind
	if(-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sa, flen)){
		perror("Sock options failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	

	if(-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)){
		perror("binding failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
   
    struct timeval timeout;
    
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int send_next = 0;
    int ack = 0;
    int remaining_space = NULL; //no data

    int select_result;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    long unsigned int position_of_file = 0;
   
    generateresponse(sock, (struct sockaddr*)&sa, flen,&send_next, &remaining_space, &position_of_file, filecontent);
    
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
	        rsize = recvfrom(sock, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&sa, &flen);
		    if(rsize < 0)
		    {
			    perror("receive from failed closing socket and exiting application...\n");
			    close(sock);
			    exit(EXIT_FAILURE);
		    }
		
	        generateresponse(sock, (struct sockaddr*)&sa, flen, &send_next, &remaining_space, &position_of_file, filecontent);
	    }
	
	    //reset select
	    FD_ZERO( &read_fds );
	    FD_SET( sock, &read_fds);
    }

    // do stuff
    return 1;
}

