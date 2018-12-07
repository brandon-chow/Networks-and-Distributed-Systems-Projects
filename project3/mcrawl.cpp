
#include <iostream>
#include <unistd.h>
#include <fstream> 
#include <string> 
#include <string.h> 
#include <thread> 
#include <queue> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h> 
#include <sys/stat.h>
#include <sys/types.h> 

using namespace std;
int cnt = 0;
queue<const char*> download_queue;

/*
    1. parse user input: mcrawl [ -n max-flows ] [ -h hostname ] [ -p port ] [-f local-
directory]
    2. communicate with http server and download files; 
    - how to parse from html?
    - structure to store pending jobs
    3. how to store files (stiches file names together?)
    4. 
*/
void print_help(){
    cout << "mcrawl [ -n max-flows ] [ -h hostname ] [ -p port ] "
            "[-f local-directory]\n";
}

void int_err(){
    fprintf(stderr,"internal error\n");
    exit(0);
}


int main(int argc, char** argv){
    int c,max_flows = -1,port = -1;
    string hostname = "", local_dir ="";
    while ((c = getopt(argc,argv,"n:h:p:f:")) != -1){
        switch(c)
        {
            case 'n':
                if (optarg) max_flows = atoi(optarg);
                break;
            case 'h':
                if (optarg) hostname = optarg;
                break;
            case 'p':
                if (optarg) port = atoi(optarg);
                break;
            case 'f':
                if (optarg) local_dir = optarg;
                break;
            default:
                fprintf(stderr, "arguments invalid\n");
                print_help();
                exit(1);
                break;
        }
    }
    //cout << hostname << endl << max_flows  << endl << local_dir << endl << port;    
    if (max_flows == -1 || port ==-1 || hostname == "" || local_dir == "")
    {
        fprintf(stderr, "arguments invalid\n");
        print_help();
        exit(1);
    }
    //create local directory
    errno = 0;
    const int dir_err = mkdir(local_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err && errno != EEXIST)
    {
        printf("Error creating directory!n");
        exit(1);
    }

    struct hostent *server;         
    server = gethostbyname(hostname.c_str());        
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
         int_err();    
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        int_err();
    download_queue.push((const char*)"index.html");
    char send_data[256], recv_data[1024];
    const char* temp = download_queue.front();
    download_queue.pop();
    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: eychtipi.cs.uchicago.edu\r\n\r\n",temp); 
    int n = 0;    

            n = sendto(sockfd, send_data, strlen(send_data), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));            
            while (1)
            {
                n = recv(sockfd,recv_data,sizeof(recv_data),0);                
                if (n ==0)
                {
                    break;
                }
                cout << recv_data;
                memset(recv_data,0,sizeof(recv_data));
            }
      

    return 0;
}


/* instruction for creating file 
    ofstream myfile;
    myfile.open(local_dir + "/foo.txt");
    myfile << "wth";
    myfile.close();
    
*/