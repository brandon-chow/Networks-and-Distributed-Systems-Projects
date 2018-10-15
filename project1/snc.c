/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
   gcc server2.c -lsocket
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>

/* function prototypes */
 /* function prototypes */

/*
1. what happens when server ctrl d
2. ip address 
3. UDP send in real time?
4. invalid argument
5.error message
*/
void serve(struct hostent*, int, int);
void client(struct hostent*, int, int);


void error(char *msg)
{
    perror(msg);
    exit(1);
}

void opt_err(){
	fprintf(stderr,"invalid or missing options\n");
    fprintf(stderr,"usage: snc [­-l] [­-u] [hostname] port\n");   
    exit(1);
}
void int_err(){
	fprintf(stderr,"internal error\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	int lflag, uflag,hflag;
    lflag = uflag = hflag = 0;
    int portno;
    struct hostent *server;         
    int c;
    while ((c = getopt (argc, argv, "lu")) != -1)
      switch (c)
      {
        case 'l':
          lflag = 1;
          break;
        case 'u':
          uflag = 1;
          break;
        case '?':
        default:
          opt_err();
          break;
      }     
    if (optind == argc) opt_err();    
    char* ch = argv[argc-1];    
    while (*ch != '\0'){
        if (isdigit(*ch) == 0) opt_err();
        ch++;
    }           
    portno = atoi(argv[argc-1]); 
    if (portno < 1025 || portno > 65535) opt_err();   
    if (argc - 2 == optind) hflag = 1;
    if (hflag == 1)  {
    	server = gethostbyname(argv[optind]);
    }
    else {   
    	server = gethostbyname(argv[optind]);    	
    	server->h_addr = INADDR_ANY;
    	server->h_length = 0;     	
    }           
    
    if (hflag == 0 && lflag == 0) opt_err();        	 
    if (lflag == 1){
    	serve(server,uflag,portno);    	
    }
    else {
    	client(server,uflag,portno);   
    }

    
    return 0;
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/


void serve(struct hostent *server, int uflag, int portno){
	int sockfd, newsockfd, clilen, pid;
    
    struct sockaddr_in serv_addr, cli_addr; 
    char buffer[256];
    if (server == NULL) {
        int_err();
    } 
    if (uflag == 1) sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    else sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
         int_err();
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
             int_err();
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    if (uflag != 1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);        
        if (newsockfd < 0) 
          int_err();
    }
    if (uflag == 1){
        while(1){
            bzero(buffer,256);    
            int n;
            n = read(sockfd,buffer,255);          
            if (n < 0) 
                 int_err();
            printf("%s",buffer);
        }
    }
    else {
        while(1){
         	bzero(buffer,256);    
         	int n;
        	n = read(newsockfd,buffer,255);
        	if (n == 0) break;
            if (n < 0) 
            	 int_err();
        	printf("%s",buffer);
        }
    }
    return;
}

void client(struct hostent *server, int uflag, int portno){
	int sockfd, n;
    struct sockaddr_in serv_addr;    
    char buffer[256];
    if (uflag == 1) sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    else sockfd = socket(AF_INET, SOCK_STREAM, 0);    
    if (sockfd < 0) 
         int_err();
    if (server == NULL) {
         int_err();
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        int_err();
    if (uflag == 1){
        while (1){
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            n = write(sockfd,buffer,strlen(buffer));
            if (n == 0) while(1);
            if (n < 0) 
                 int_err();
         }
    }
    else {
        while (1){
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            n = write(sockfd,buffer,strlen(buffer));
            if (n == 0) break;
            if (n < 0) 
                 int_err();
         }
    }
    return;
}
