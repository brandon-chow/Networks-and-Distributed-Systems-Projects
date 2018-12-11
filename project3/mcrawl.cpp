
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
int count = 0;
//download_queue for const char and store parse from input; set for string; need convertion 
queue<string> download_queue;
set<string> crawled;
struct hostent *server;         
int c,max_flows = -1,port = -1;
string hostname = "", local_dir ="";
string delimiter = ".";
mutex mtx;
struct sockaddr_in serv_addr;
ofstream myfile;
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
    download_queue.push("index.html");
    server = gethostbyname(hostname.c_str());        
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    myfile.open(local_dir + "/foo.txt");
    crawler();
    myfile.close();
    cout << count << endl;
    return 0;
}

int compare(char* c, int len, string cmp){
    int cnt = 0;
    for (int i = 0; i < len; i++)
    {
        if (*c == '\0') return 0 - cnt;
        else if(*c != cmp.at(i))
            return 0;
        c++;
        cnt++;
    }
    while (*c != '\0' && *c != '"')
    {
        cnt++;
        c++;
    }
    if (*c == '\0') return -cnt;
    else return cnt;
}

string parse_file(char* to_parse, size_t len){
    size_t cur = 0;
    bool flag = false;    
    int len_of_key = 0;
    string ret = "";
    char* temp;
    char* pos = to_parse;
    while (cur < len)
    {
        //cout << len << ": ";
        //cout << strlen(to_parse) << endl;
        flag = false;
        char c = *pos;
        //scout << *pos;  
        //cout << c;
        //cout << c;
        switch (c)
        {            
            case 'H':
            //href\0
                len_of_key = compare(pos,6,"HREF=\"");
                if (len_of_key > 0)
                {
                    flag = true;
                    cur += len_of_key+1;
                    temp = pos + len_of_key;
                    len_of_key -= 6;
                    pos += 6;
                }
                else if (len_of_key < 0) 
                {
                    ret = string(pos);
                    break;
                }
                break;
            case 'h':
                len_of_key = compare(pos,6,"href=\"");
                if (len_of_key > 0)
                {
                    flag = true;
                    cur += len_of_key+1;
                    temp = pos + len_of_key;
                    len_of_key -= 6;
                    pos += 6;
                }
                else if (len_of_key < 0)  {
                    ret = string(pos);
                    break;
               
                }
                break;
            case 's':
                len_of_key = compare(pos,5,"src=\"");
                if (len_of_key > 0)
                {
                    flag = true;
                    cur += len_of_key+1;
                    temp = pos + len_of_key;                    
                    len_of_key -= 5;
                    pos += 5;
                }
                else if (len_of_key < 0)  {
                    ret = string(pos);
                    break;
                }
            case 'S':
                len_of_key = compare(pos,5,"SRC=\"");
                if (len_of_key > 0)
                {
                    flag = true;
                    cur += len_of_key+1;
                    temp = pos + len_of_key;                    
                    len_of_key -= 5;
                    pos += 5;
                }
                else if (len_of_key < 0)  {
                    ret = string(pos);
                    break;
                }
                break;
            default:
                flag = false;
        }
        if (flag == false) {
            pos++;
            cur += 1;
        }
        else 
        {  
        *temp = '\0';
        char* new_file = strdup(pos);                
        pos = temp + 1;
        mtx.lock();
        download_queue.push(string(new_file));
        mtx.unlock();            
      //  cout << new_file << endl;
        }        
    }
   // if (ret != "")
    // << ret << endl;
    return ret;
}

void crawl_html(string to_crawl,int sockfd){
	// crawl from html, need to parse "to_crawls" from the html and add them to queue
	char send_data[256], recv_data[1024];
    int len_of_key = 0;
    string temp_s;
    const char* c_hostname = hostname.c_str();
    const char* c_to_crawl = to_crawl.c_str();
    int size = sizeof(recv_data);
    //cout << size <<endl; 
	snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",c_to_crawl,c_hostname); 
	int n = 0;        
  	n = sendto(sockfd, send_data, strlen(send_data), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));            
    while (1)
    {
        *(recv_data+ len_of_key) = 'a';
        myfile << recv_data << endl;
        n = recv(sockfd,len_of_key + recv_data,size - len_of_key,0); 
        n += len_of_key;
     //   cout <<  recv_data << endl << "----------"<<endl ;
        if (n == 0)        
            break;
        
        temp_s = parse_file(recv_data, n);
        len_of_key = temp_s.size();
       // cout << len_of_key << endl;
        if (len_of_key > 0)
        {
            snprintf(recv_data, size, "%s",temp_s.c_str()); 
          //  cout << recv_data << endl;
        }
    }
}

void crawl_file(string to_crawl,int sockfd){
	// download from file 
    return;
}

void crawler(){
	string to_crawl;
    string::size_type r_pos, l_pos;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
         int_err();    
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        int_err();

	while (1){
		mtx.lock();
		if (download_queue.empty()) return;
		to_crawl = download_queue.front();
		download_queue.pop();
		mtx.unlock();
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
            count++;
            //myfile << to_crawl << endl;
			crawl_html(to_crawl,sockfd);
        }
		else {
            count++;
         //   myfile << to_crawl << endl;
            crawl_file(to_crawl,sockfd); 
	    }
    }
}
/* instruction for creating file 
    ofstream myfile;
    myfile.open(local_dir + "/foo.txt");
    myfile << "wth";
    myfile.close();
    
*/