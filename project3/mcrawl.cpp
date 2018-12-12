
#include <iostream>
#include <unistd.h>
#include <fstream> 
#include <string> 
#include <string.h> 
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
#include <pthread.h>
#include <vector>
#include <thread>

using namespace std;
int count = 0;
//download_queue for const char and store parse from input; set for string; need convertion 
queue<string> download_queue;
set<string> crawled;
struct hostent *server;         
int c,max_flows = -1,port = -1;
string hostname = "", local_dir ="";
mutex mtx,mtx1,mtx2,mtx3,mtx4;
struct sockaddr_in serv_addr;
ofstream myfile;
bool cook = false; 
int running = 0;
string cookie = "";
set<string> used_name;
 

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
    download_queue.push("dynamics/");
    server = gethostbyname(hostname.c_str());        
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    myfile.open(local_dir + "/foo2.txt");
    thread  threads[max_flows];
    for (int i = 0; i < max_flows; i++)
    {
        threads[i] = thread(crawler);
    }
    for (int i = 0; i < max_flows; i++)
    {
        threads[i].join();
    }
    cout << count;
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
int compare_cook(char* c, int len, string cmp){
    int cnt = 0;
    for (int i = 0; i < len; i++)
    {
        if (*c == '\0') return 0 - cnt;
        else if(*c != cmp.at(i))
            return 0;
        c++;
        cnt++;
    }

    while (*c != '\0' && *c != ';')
    {
        cnt++;
        c++;
    }
    if (*c == '\0') return -cnt;
    else {
       return cnt;
    }
}

string parse_file(char* to_parse, size_t len,char cookie[512]){
    size_t cur = 0;
    bool flag = false;    
    int len_of_key = 0;
    string ret = "";
    bool cook = (cookie[0] != '\0');
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
        if (cook == false && c == 'S')
        {
            len_of_key = compare_cook(pos,12,"Set-Cookie: ");
                if (len_of_key >0)
                {
                    flag = true;
                    cur += len_of_key+1;
                    temp = pos + len_of_key;
                    len_of_key -= 12;
                    pos += 12;
                    *temp = '\0';
                    char* new_file = strdup(pos);                
                    pos = temp + 1;
                    cookie[0] = 'a';
                    snprintf(cookie, 512, "%s",new_file); 
                    //cout << new_file << endl;
                    cook = true;
                    continue;
                }
                else if (len_of_key < 0) 
                {
                    ret = string(pos);
                    break;
            }
        }
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
        mtx3.lock();
        if (crawled.count(new_file) != 0) 
        {
            mtx3.unlock();
            break; //check if crawled
        }
        mtx3.unlock();
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

void crawl_html(string to_crawl,int sockfd,char cookie[512]){
	// crawl from html, need to parse "to_crawls" from the html and add them to queue
    cout << cookie << endl;
    char send_data[256], recv_data[1024];
    string::size_type r_pos;
     string filename,temp;
    int len_of_key = 0;
    string temp_s;
    
    
    const char* c_hostname = hostname.c_str();
    const char* c_to_crawl = strdup(to_crawl.c_str());
     
    while(to_crawl.at(to_crawl.size()-1) == '/')
        to_crawl = to_crawl.substr(0,to_crawl.size()-1);
    r_pos =   to_crawl.rfind('/');
    if (r_pos != string::npos)
        to_crawl = to_crawl.substr(r_pos+1,to_crawl.size()-r_pos);
    int size = sizeof(recv_data)-1;
    //cout << size <<endl;  
    mtx4.lock();
    temp = to_crawl;
    int i = 1;
    while (used_name.count(to_crawl) > 0)
    {
        to_crawl =  to_string(i) + '-' + temp;
        i++;
    }
    used_name.insert(to_crawl);
    mtx4.unlock();
    filename = local_dir + "/" +to_crawl;
    ofstream file;
    file.open(filename); 
    if (cookie[0] == '\0')
        snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",c_to_crawl,c_hostname); 
	else
        snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\nCookie: %s;\r\n\r\n",c_to_crawl,c_hostname,cookie); 
        
    cout << send_data << endl;
  
       
   // cout << send_data << endl;
    int n = 0;        
  	n = sendto(sockfd, send_data, strlen(send_data), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); 
    //cout << send_data<< endl;          
    while (1)
    {
        len_of_key =0;
        *(recv_data+ len_of_key) = 'a';
        n = recv(sockfd,len_of_key + recv_data,size - len_of_key,0); 
        // cout << recv_data << endl;

        n += len_of_key;
     //   cout <<  recv_data << endl << "----------"<<endl ;
        recv_data[n] = '\0';
        file << recv_data+len_of_key;
        if (n == 0)        
            break;
        
        temp_s = parse_file(recv_data, n, cookie);
        len_of_key = temp_s.size();
       // cout << len_of_key << endl;
        if (len_of_key > 0)
        {
            snprintf(recv_data, size, "%s",temp_s.c_str()); 
          //  cout << recv_data << endl;
        }
        memset(recv_data+len_of_key,0,size+1-len_of_key);
    }
    file.close();
}

void crawl_file(string to_crawl,int sockfd,char cookie[512]){
	// download from file 
    return;
}

void crawler(){
	string to_crawl;
    string::size_type r_pos, l_pos, pos;
    char cookie[512];
    cookie[0] = '\0';
    //- ./clickstream/index.html -http / - ../ - ./  - # // 

   while (1){
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
         return;    
        if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
         return;
		mtx.lock();
		if (download_queue.empty()) 
        {
            mtx1.lock();
            if (running > 0)
            {
               mtx1.unlock();
               mtx.unlock();
               continue;
            }
            else
            {
                mtx1.unlock();
                mtx.unlock();
                return;
            }
        }
        mtx1.lock();
        running++;
        mtx1.unlock();
		to_crawl = download_queue.front();
		download_queue.pop();
		mtx.unlock();
	    if (to_crawl.at(0) == '#')
        {
            mtx1.lock();
            running--;
            mtx1.unlock();
            continue;
        }
        l_pos = to_crawl.find("http");
        if (to_crawl.at(0) == 'h')
        {
            mtx1.lock();
                running--;
                mtx1.unlock();
                continue;

        }
        if (l_pos != string::npos)
        {
            if (to_crawl.find(hostname) == string::npos)
            {
                mtx1.lock();
                running--;
                mtx1.unlock();
                continue;
            }  
        }      
        while (to_crawl.at(0)  == '.' || to_crawl.at(0)  == '/')
            {
             to_crawl = to_crawl.substr(1,to_crawl.size()-1);
        }
        mtx3.lock();
        if (crawled.count(to_crawl) != 0) 
        {
            mtx3.unlock();
            continue; //check if crawled
        }
        crawled.insert(to_crawl);        
        mtx3.unlock();  
        string file_type = "";
        pos = to_crawl.rfind('.');
        if (pos != string::npos)
	    	file_type = to_crawl.substr(pos+1,to_crawl.size()-pos);
        cout << to_crawl << " "  << endl;
        if (file_type == "htm" || file_type == "html" || to_crawl.substr(to_crawl.size()-1,1).compare("/") ==0 ){
            //myfile << to_crawl << endl;

			crawl_html(to_crawl,sockfd,cookie);
        }
		else {
         //   myfile << to_crawl << endl;
            crawl_file(to_crawl,sockfd,cookie); 
	    }
        mtx2.lock();
        count++;
        mtx2.unlock();        
        mtx1.lock();
        running--;
        mtx1.unlock();
        close(sockfd);
    }

}
/* instruction for creating file 
    ofstream myfile;
    myfile.open(local_dir + "/foo.txt");
    myfile << "wth";
    myfile.close();
    
*/