//Libraries
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/types.h>
#include<netinet/in_systm.h>
#include<netinet/udp.h>
#include<stdint.h>
#include<sys/select.h>
#include<sys/time.h>
#include<signal.h>
#include<fcntl.h>
#include<vlc/vlc.h>

#define MAXLINE 40480
#define SA struct sockaddr

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

 // to use the exit() function
extern void exit (int code);

enum {
		TEARDOWN =-1 , PAUSE , PLAY , REPOSITION
		};


char FileName[32] = "output.mp4";
int status=5; //any number less than -1 and greater than 2 

struct HTTP_pkt{
	int start , end , acceptd_start , accepted_end , status; 
	char buff[MAXLINE];
	};
	char buff[MAXLINE];

//*Variable decleration*//
	int sockfd , connfd , rd , wt ,sz , threshold ,com_sz=0 ,new_location,start,end,falg;
	struct sockaddr_in servaddr;
	char buff[MAXLINE], Request[MAXLINE];
	struct HTTP_pkt http_packet; 
	FILE *fp; 

//////////////////////////////////////////////////////////////Set up Media Player			
    	libvlc_instance_t * inst;
     	libvlc_media_player_t *mp;
     	libvlc_media_t *m;
     

void * rcv_http (void * arg) {

	memset(&http_packet, 0, sizeof(http_packet));
	printf(RESET "START RECEIVING\n");

	while (rd =recv(sockfd, &buff, sizeof(buff),0) ){
		
		if (status == PAUSE) printf("PUSE : %s\n",buff);
		com_sz+=rd; 
		if (com_sz == sz/2) sleep(2);
		if (status == PAUSE) 	{fclose(fp); fp=fopen(FileName,"w");}

		if (com_sz == sz || com_sz<start || com_sz>=end) {
			
		 break;}

		fwrite(buff, 1, rd, fp);
		//memset(&http_packet, 0, sizeof(http_packet));
		}	

	fclose(fp);
	
	if (com_sz >= sz)printf(RESET "DONE RECEIVING\n");		

	pthread_exit(NULL);
}

int main(int argc , char * argv[]){


	//check of number of arguments
	if(argc != 3){ 
	puts("./Server.out <Server IP> <TCP Port>  ");
	exit(1);}


	//create the socket
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if (sockfd < 0) {perror("Socket error");
			exit(1);}
	//clear
	memset(&servaddr , 0 , sizeof(servaddr));

	//ipv4
	servaddr.sin_family = AF_INET ;
	servaddr.sin_port = htons(atoi(argv[2]));

	//check the ip
	if(inet_pton(AF_INET , argv[1] , &servaddr.sin_addr) <= 0) { 
		 perror("inet_pton error < enter a correct format of ip address > : ");
		 exit(1);}

	//connecting 
	connfd = connect(sockfd , (SA*) &servaddr , sizeof(servaddr));
	if(connfd < 0) { 
	perror("Connect error: ");
		exit(1);}


	char file[MAXLINE];
	printf( BLU "\t\t** WElome to VLC **\n");
	printf(RESET "\tPlease enter video name :\n\n");	   
	scanf("%s", file);

	if (wt=write(sockfd,&file, strlen(file)) < 0) {
		perror("Sendeing the vedio name faild"); }


	//Recieve Response  about the requested video 
	memset(&http_packet, 0, sizeof(http_packet));

	rd =recv(sockfd, &http_packet, sizeof(http_packet),0);
	if (rd<= 0){ 
		perror("Recieving msg error "); }

	//For debug
	//printf("status : %d \n",http_packet.status);
	//printf("Expected SIZE : %d \n ", http_packet.end);
	sz=http_packet.end;
	
	if (http_packet.status == 404 ) {printf(RED " FILE : " RESET "%s" RED" NOT EXIST :( 404 \n \t\t\t\t BYE....\n",file); 
			    exit(1);}
		else printf(YEL " FILE : " RESET"%s" YEL " IS EXIST :D OK \n\n",file);

	//// Open file pre
	fp=fopen(FileName,"w");
     	/* Load the VLC engine */
     	inst = libvlc_new (0, NULL);
  
     	/* Create a new item */
     	m = libvlc_media_new_path (inst,"output.mp4");
        
     	/* Create a media player playing environement */
     	mp = libvlc_media_player_new_from_media (m);
     
     	/* No need to keep the media now */
     	//libvlc_media_release (m);
 	
	////////////////////////////////////////////////////Recieve
	pthread_t pid; 
	

	/////////////////////////////////////////Menue
     	while (status != TEARDOWN) {

		printf(RESET "\n\nPlease Choose an Action:\n1: Play\n2: Pause\n3: TearDown\n4: Reposition\n\n");
		char choice;
		scanf(" %c", &choice);

		if (choice == '1') {//choice = 1
			if (status == PLAY) {
				puts("State is already PLAY!!");
				continue;
			}

			memset(&http_packet, 0, sizeof(http_packet));			
			status=PLAY;
			//Get maximum possiable from server
			http_packet.status=PLAY;
			start=http_packet.start=1;
			end=http_packet.end=sz;
			printf("END : %d\n",end);

			wt =send(sockfd, &http_packet, sizeof(http_packet), 0);
			if (wt < 0 ) perror("Write error : ");
			
			pthread_create(&pid , NULL , rcv_http,NULL); 
			//sleep(2);
			printf("SIZE : %d\n",com_sz);

			/* play the media_player */
     			libvlc_media_player_play (mp);
			
				} //choice = 1 end  
		else if (choice == '2') { //choice = 2
			if (status == PAUSE) {
				puts("State is already PAUSE!!");
				continue;
			}

			status=PAUSE;
			memset(&http_packet, 0, sizeof(http_packet));
			//HTTP idea of stop, 
			http_packet.status=PAUSE;
			http_packet.start=com_sz;
			http_packet.end=com_sz;
			start=end=com_sz;
			
			wt =send(sockfd, &http_packet, sizeof(http_packet), 0);

			if (wt < 0 ) perror("Write error : ");
			
			/* Stop playing */

			http_packet.status=PAUSE;
			http_packet.start=com_sz;
			http_packet.end=sz;
			start=com_sz;
			end=sz;
			
			wt =send(sockfd, &http_packet, sizeof(http_packet), 0);

			if (wt < 0 ) perror("Write error : ");
			
			} //choice 2 end  
		else if (choice == '3') { //choice = 3
			status=TEARDOWN;			
			memset(&http_packet, 0, sizeof(http_packet));
			http_packet.status=TEARDOWN;
			wt =send(sockfd, &http_packet, sizeof(http_packet), 0);
			if (wt < 0 ) perror("Write error : ");			
			
			/* Stop the media_player */
     			//libvlc_media_player_stop(mp);
			close(connfd);
			exit(0);
			
		} //end choice 3 
	 	else if (choice == '4') { //choice 4 
		
		printf("Insert, new position in seconds:  \n ");
		scanf("%d",&new_location);
			//memset(&http_packet, 0, sizeof(http_packet));
			http_packet.status=REPOSITION;
			http_packet.start=new_location;
			wt =send(sockfd, &http_packet, sizeof(http_packet), 0);
			if (wt < 0 ) perror("Write error : ");

		} // end choice 4 

		else { //Bad insert
			
		printf(":( , TRY AGAIN");
		continue; 
			}	//end bad instert
	
		}//while (status != TEARDOWN)

    
return 0;
}
