/*
	Server: Written by Aiah AbuDouleh 107988
		Multimedia Project

*/

#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <vlc/vlc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#define MAXLINE 40480
#define SA struct sockaddr
#define LISTENQ 1024
#define BKT_SIZE 81920

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

extern void exit (int code); // to use the exit() function

enum {
		TEARDOWN =-1 , PAUSE , PLAY , REPOSITION
		};

struct HTTP_pkt{
	int start , end , acceptd_start , accepted_end , status; 
	char buff[MAXLINE];
	};


//*Networking identifires.*// 
int listenfd, connfd,len,data,start,end,status;
struct sockaddr_in  TCP_SERVADDR;
struct sockaddr_in CliAddr;
struct HTTP_pkt http_packet; 
struct stat st;
char cli_req[MAXLINE];
char cli_response[MAXLINE], Query[MAXLINE]; 
char file_requested[MAXLINE],path[MAXLINE];
bool IsFound; 
unsigned short seq;
char buff[MAXLINE];

void * send_http (void * arg) {

	FILE *fp; 
	fp = fopen(file_requested,"r");	
	
	int sz=0;
		
	while (data = fread(buff,1,sizeof(buff),fp)){
	if (status == TEARDOWN) break;

	sz+=data;
	http_packet.end=sz;

	if (sz >= start && sz<= end) send(connfd, buff, data, 0);

	//if (sz<start || sz> end) break; 
	memset(&http_packet, 0, sizeof(http_packet)); 
	http_packet.start=data;
	}		

	pthread_exit(NULL);
}


int main(int argc, char * argv[]){

	if (argc != 2) {
		puts("./Server.out <TCP Port>");
		exit(0);
		}



	listenfd=socket(AF_INET,SOCK_STREAM,0);
	if (listenfd < 0) {
		perror("Listen socket: ");
		exit(0);
		}

	
	bzero(&TCP_SERVADDR, sizeof(TCP_SERVADDR));
	TCP_SERVADDR.sin_family = AF_INET;
	TCP_SERVADDR.sin_addr.s_addr = htonl(INADDR_ANY);
	TCP_SERVADDR.sin_port = htons(atoi(argv[1]));


	if (bind(listenfd, (SA *) &TCP_SERVADDR, sizeof(TCP_SERVADDR)) < 0 ){
		perror("Bind error: ");
		exit(0);}

	if(listen(listenfd, LISTENQ) < 0 ) {
		perror("Listen queue: ");
		exit(0);
		}


	printf("\n\t\t\t" CYN " ** VLC Media Server ** \n\n");

	while (1){

		printf(MAG "\t\t Waiting Client request....\n");

		connfd = accept(listenfd,(struct sockaddr *) &CliAddr, &len);
		if (connfd < 0) printf (RED "Accept Error :(\n"); 
			else printf(GRN "\tClient Accepted, waiting a file (.mpg) request...\n\n");

		
		//Recieve a file path
		memset(&http_packet, 0, sizeof(http_packet));
		data = read(connfd,file_requested, MAXLINE);

		//Check file if found = send ok , and file size 
		if (access(file_requested, F_OK) != -1)
				{
				stat(file_requested, &st);
				http_packet.status=200;
				http_packet.start=1;
				http_packet.end=(int)st.st_size;
				IsFound=1;
				start=1;	
				end=(int)st.st_size;}
			else
				{
				http_packet.status=404;	
				IsFound = 0;}

		
		//send reply about the file
		data =send(connfd, &http_packet, sizeof(http_packet), 0);
		if (data < 0 ) perror("Write error : ");
		
		//printf("Found : %d\n",IsFound);
		if (IsFound == 0) continue; 

		printf(RESET "Size : %d\n",(int)st.st_size);

		memset(&http_packet, 0, sizeof(http_packet));
		http_packet.start=1;
			 
		//Sending Media file Here ........///////////////////////////////////////////////////
		pthread_t pid; 
		pthread_create(&pid , NULL , send_http,NULL);
		
		///Requsting loop 

		fd_set rset;
		FD_ZERO(&rset);

		// Not Completed yet 
		while(1){
		
			FD_SET(connfd,&rset);
			int max=connfd+1;

			int n = select(max, &rset, NULL, NULL, NULL);
			if (n < 0)
				perror("select error");

			if (FD_ISSET(connfd,&rset)){//FD_ISSET
			
			memset(&http_packet, 0, sizeof(http_packet));
			data = read(connfd,&http_packet, sizeof(http_packet));

			start=http_packet.start; 
			end=http_packet.end;
			status=http_packet.status;
 
			if (data == 0) break;
			printf("Status: %d \n",http_packet.status);
			
				} //FD_ISSET
					}//While 1 _ inner
		

								}//while 1 	

	return 0;}
