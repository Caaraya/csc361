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

// TCP Server
/*
http://stackoverflow.com/questions/12730477/close-is-not-closing-socket-properly
http://stackoverflow.com/questions/8872988/properly-close-a-tcp-socket?rq=1
http://www.linuxhowtos.org/C_C++/socket.htm

*/
void sendMsg(int sock,char *sentstr,struct sockaddr* saptr, socklen_t flen)
{
	//send request(do we need to handle large files?)
	if (sendto(sock,sentstr,strlen(sentstr),0, saptr, flen)==-1) 
	{
		perror("sending failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
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
   if(argc != 4){
		printf("need program, receiver ip, receiver port, and filename, incorrect amount of arguments");
		return 0;
	}
    
    //initialize socket data
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa;

	memset(&sa,0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &(sa.sin_addr));

	socklen_t flen;
	ssize_t rsize;
	char buffer[8][1024];
 
   char* filecontent = getFileContent(argv[3]);
   
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
			rsize = recvfrom(sock, (void*)buffer[bufferAvail], sizeof(buffer[bufferAvail]), 0, (struct sockaddr*)&sa, &flen);
			printf(buffer[bufferAvail]);
		    
		}
		
		//reset select
		FD_ZERO( &read_fds );
		FD_SET( sock, &read_fds);
    }
 
   //do stuff
   return 1;
}
