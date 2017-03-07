#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>

int getDirectory(char *cwd, int sock)
{
	//use stat.h to check if the directory exists
	struct stat sb;
	if (!(stat(cwd, &sb) == 0 && S_ISDIR(sb.st_mode)) || cwd[strlen(cwd)-1] == '/')
	{
		return 0;
	}
	return 1;
}
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
	if (sendto(sock,sentstr,strlen(sentstr),0, saptr, flen)==-1) 
	{
		perror("sending failed closing socket and exiting application...\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
}

void generateResponse(char **req, int sock, struct sockaddr_in sa, socklen_t flen, char *cwd, char* request)
{
	char response[1024] = {0};
	char sentstr[1024];
	char* filecontent;
	long fileSize;
	char dir[200];
	if(req[2] == NULL)
	{
		strcpy(response, "HTTP/1.0 400 Bad Request\r\n");
		strcpy(sentstr, response);
	}
	else
	{
		//check format of response
		if(strcmp(req[2], "HTTP/1.0\r\n\r\n") != 0 || strcmp(req[0], "GET") != 0)
		{
			strcpy(response, "HTTP/1.0 400 Bad Request\r\n");
			strcpy(sentstr, response);
			sendMsg(sock,sentstr,(struct sockaddr*)&sa, flen);
		}
		else
		{
			strcpy(dir, cwd);
			strcat(dir, req[1]);
			if(dir[strlen(dir) - 1] == '/')
			{
				strcat(dir, "index.html");
			}
			//check if file exists
			FILE * f = fopen(dir, "rb");
			if (f == NULL)
			{
				strcpy(response, "HTTP/1.0 404 Not Found\r\n");
				strcpy(sentstr, response);
				sendMsg(sock,sentstr,(struct sockaddr*)&sa, flen);
			}
			else
			{
				//generate response
				strcpy(response, "HTTP/1.0 200 OK");
				strcpy(sentstr, response);
				strcat(sentstr, "\n\n");
				strcat(response, "; ");
				strcat(response, dir);
				strcat(response, "\n\n");
				
				//get file content from file stream
				fseek(f, 0, SEEK_END);
				fileSize = ftell(f);
				rewind(f);
				filecontent = malloc(fileSize * (sizeof(char)));
				int filebuffsize = fileSize * (sizeof(char));
				fread(filecontent, sizeof(char), fileSize, f);
				fclose(f);
				
				//handle file content depending on file size
				if((strlen(sentstr) + strlen(filecontent)) <= sizeof(sentstr))
				{
					strcat(sentstr, filecontent);
					sendMsg(sock,sentstr,(struct sockaddr*)&sa, flen);
				}
				else if(strlen(filecontent) <= sizeof(sentstr))
				{
					sendMsg(sock,sentstr,(struct sockaddr*)&sa, flen);
					sendMsg(sock,filecontent,(struct sockaddr*)&sa, flen);
				}
				else
				{
					//handle large files
					sendMsg(sock,sentstr,(struct sockaddr*)&sa, flen);
					int index = 0;
					while(index < filebuffsize)
					{
						sendto(sock,&filecontent[index],sizeof(sentstr),0, (struct sockaddr*)&sa, flen);
						index += sizeof(sentstr);
					}
				}
				
			}
		}
	}
	
	const char * months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Augt", "Sept", "Oct", "Nov", "Dec"};
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	printf("%s %d %d:%d:%d %s:%d %s; %s", months[tm.tm_mon], tm.tm_mday,  tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), request, response);
}

int main(int argc, char *argv[])
{
	if(argc != 3){
		printf("need program port and directory, incorrect amount of arguments");
		return 0;
	}

	//initialize socket data
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa;

	memset(&sa,0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(atoi(argv[1]));

	socklen_t flen;
	ssize_t rsize;
	char buffer[1024];
	char cwd[1024];

	//construct path from current working directoy and argument
	getcwd(cwd, sizeof(cwd));
	strcat(cwd, argv[2]);

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

	
	//check if directory exists
	if(!getDirectory(cwd, sock))
	{
		perror("directory not found or incorrect format with trailing / ..\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	printf("%s is running on UDP port %s and serving %s\n", argv[0], argv[1], argv[2]);
	printf("press 'q' to quit ...\n");
	
	//setup select parameters and helper objects
	fd_set read_fds;
	char character[80];
	FD_ZERO( &read_fds );
	FD_SET( STDIN_FILENO, &read_fds );
	FD_SET( sock, &read_fds);

	while(1){
		

		if(select(sock + 1,&read_fds, 0, 0, 0) == -1)
		{
			perror("select call failed closing socket and exiting application...\n");
			close(sock);
			exit(EXIT_FAILURE);
		}
		if(FD_ISSET(STDIN_FILENO, &read_fds))
		{
			//handle q press
			scanf("%s", character);
			if(strcmp(character, "q") == 0)
			{
				close(sock);
				return 0;
			}
			else
			{
				printf("unrecognized command");
			}
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
			char **result = NULL;
			result = separateString(buffer, " ");
			char request[1024];
			
			//build initial request
			if(result[2] != NULL)
			{
				strcpy(request, result[0]);
				strcat(request, " ");
				strcat(request, result[1]);
				strcat(request, " ");
				strcat(request, result[2]);
			}
			else
			{
				strcpy(request, buffer);
			}
			//parse response log message and send response
			generateResponse(result, sock, sa, flen, cwd, request);
			memset(buffer,0 , sizeof(buffer)*sizeof(char));
			
			free(result);
			result = NULL;
			
		}
		//reset select
		FD_ZERO( &read_fds );
   		FD_SET( STDIN_FILENO, &read_fds );
		FD_SET( sock, &read_fds);
		
	}
}
