
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
#include <set>
#include <mutex>

using namespace std;
int cnt = 0;
//download_queue for const char and store parse from input; set for string; need convertion 
queue<const char*> download_queue;
set<string> crawled;
struct hostent *server;         
int c,max_flows = -1,port = -1;
string hostname = "", local_dir ="";
string delimiter = ".";
mutex mtx;
struct sockaddr_in serv_addr;

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

void crawler();

int main(int argc, char** argv){
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
    download_queue.push((const char*)"index.html");
    server = gethostbyname(hostname.c_str());        
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    crawler();
    return 0;
}

bool compare(char* c, int len, string cmp){
    for (int i = 0; i < len; i++)
    {
    
        if (*(c+i) == '\0' || *(c+i) != cmp.at(i))
            return false;
    }
    
    return true;
}

char* parse_file(char* to_parse, size_t start, size_t len){
    size_t cur = 0;
    bool flag = false;    
    while (cur < len)
    {

        char* pos = to_parse + start + cur;
        char c = *pos;
        //scout << *pos;  
       // cout << c << endl;
        //cout << c;
            
        switch (c)
        {            
            case 'H':
                if (compare(pos,4,"HREF"))
                {
                    flag = true;
                    cur += 6;
                    pos += 6;
                }
                break;
            case 'h':
                if (compare(pos,4,"href"))
                {
                    flag = true;
                    cur += 6;
                    pos += 6;

                }
                break;
            case 's':
                if (compare(pos,3,"src"))
                {
                    flag = true;
                    cur += 5;
                    pos += 5;
                }
                break;
            default:
                flag = false;
        }
        if (flag == false) cur += 1;
        else 
        {            
         
            char* temp = pos;

            while (*temp != '\0' && *temp != '"'){
                temp += 1;      
            }
            if (*temp == '\0') 
                {

                    return pos;
                }
            else
            {   
                *temp = '\0';
                char* new_file = strdup(pos);                
                pos = temp + 1;
                mtx.lock();
                download_queue.push(new_file);
                mtx.unlock();            
                //cout << new_file << endl;
            }
        }        
    }
    return NULL;
}

void crawl_html(string to_crawl,int sockfd){
	// crawl from html, need to parse "to_crawls" from the html and add them to queue
	char send_data[256], recv_data[30];
    char* next;
    char temp_buff[1024];
    char* temp_ptr = &temp_buff[0];
    const char* c_hostname = hostname.c_str();
    const char* c_to_crawl = to_crawl.c_str();
	snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",c_to_crawl,c_hostname); 
	int n = 0;        
  
    memset(temp_buff,0,sizeof(temp_buff));
	n = sendto(sockfd, send_data, strlen(send_data), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));            
    while (1)
    {
        size_t start = 0;
        n = recv(sockfd,recv_data,sizeof(recv_data),0);                        
        if (n == 0)        
            break;
        string received = string(recv_data);  
        if (temp_ptr != &(temp_buff[0]))
        {        
            char* next = &recv_data[0];
            while (*next != '"')
            {
                *temp_ptr = *next;
                next += 1;
                temp_ptr += 1;
                start += 1;
            }            
            *(temp_ptr++) = '\0';
                        cout <<   temp_buff << endl;
            mtx.lock();
            download_queue.push(temp_buff);
            mtx.unlock();    
           // cout << temp_buff << endl;           
            memset(temp_buff,0,sizeof(temp_buff));             
            temp_ptr = &temp_buff[0];        
        }        
        //cout << recv_data;
        next = parse_file(recv_data, start, n);
        if (next != NULL)
        {
            //cout << next <<endl;
          
            while (*next != '\0')
            {
                *temp_ptr = *next;
                temp_ptr++;
                next++;
            }
        }	   
    }
}

void crawl_file(string to_crawl,int sockfd){
	// download from file 
    return;
}

void crawler(){
	const char* temp;
    string::size_type r_pos, l_pos;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
         int_err();    
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        int_err();

	while (1){
		mtx.lock();
		if (download_queue.empty()) return;
		temp = download_queue.front();
		download_queue.pop();
		mtx.unlock();
		string to_crawl = string(temp);
        if (to_crawl.compare("#") == 0)
            continue;
        l_pos = to_crawl.find("http://");
        if (l_pos != string::npos)
        {

            l_pos += 7;                    
            r_pos = to_crawl.find("/",l_pos);
            string h = to_crawl.substr(l_pos,r_pos-l_pos+1);
            if (h.compare(hostname) != 0)
                continue;
        }
		//convert char* to string
		if (crawled.count(to_crawl) != 0) continue; //check if crawled
		crawled.insert(to_crawl);        
        string::size_type pos = to_crawl.rfind('.');
		string file_type = to_crawl.substr(pos+1,to_crawl.size()-pos);


        if (file_type == "htm" || file_type == "html"){
			crawl_html(to_crawl,sockfd);
        }
		else crawl_file(to_crawl,sockfd); 
	}
}
/* instruction for creating file 
    ofstream myfile;
    myfile.open(local_dir + "/foo.txt");
    myfile << "wth";
    myfile.close();
    
*/