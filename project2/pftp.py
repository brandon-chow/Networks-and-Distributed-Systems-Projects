#!/usr/bin/env python3

'''
1. parse argument / from config
2. how protocles work with socket interface
3. how to retrieve from server; create a local object
5. username/ password?
6. log
7. paralell 
8. error
---
1-can't connenct to server ; 2 - authentication faile; 3 - file not found; 
4-syntax error; 5 - command not implemented by server; 6- operatortion not allowed   
7 - generic error
---
config:
each line in the config-file specifies the login, password, hostname 
and absolute path to the file. Each line should be separated by a Line Feed (LF) character.
username,server,name,file-path,password
'''
import argparse
import socket 
import threading
import sys
port = 21
log = ''



def read_from_server(server,username,file,password,flag=0,segment=None):
    '''
    TCP
    1.cotrol link TCP:
    S->C server establishes
    C->S USER name
    S->C OK? then pass
    c->s PASS
    2.transfer link(passive)
    ---
    1. tcp to server: port  ; 
    if 220(authorized user only):
        send username
        if 230 good
        elif 331:
            send password
            if 230 go 
     230

     send TYPE A
     if 200: good

     send PASV
     (h1,h2,h3,h4,p1,p2)

     conenct to p1*256+p2

     send QUIT
    '''
    global log,port

    sc_ctrl = socket.socket(socket.AF_INET,socket.SOCK_STREAM)    
    try: 
        ip_server = socket.gethostbyname(server)
        sc_ctrl.connect((ip_server,port))
    except:
        sys.stderr.write("can't connenct to server")
        exit(1)
    msg = None
    msg_send = None
    size = -1
    while True:
        try:         
            # remeber to write log and error handler (such as host name incorrect blahblah)           
            msg = sc_ctrl.recv(128).decode()        
              
            if log is not None:
               log.write('S->C: '+ msg)   

            if int(msg[0:3]) == 220:
                msg_send = ('USER ' + username + '\n').encode()
            elif int(msg[0:3]) == 331:
               msg_send = ('PASS ' + password + '\n').encode()
            elif int(msg[0:3]) == 230:
                if flag == 0 or flag == 2:
                    msg_send = ('TYPE I' + '\n').encode()
                elif flag == 1:
                    msg_send = ('STAT ' + file + '\n').encode()
            elif int(msg[0:3]) == 200:
                msg_send = ('PASV' + '\n').encode()      
            elif int(msg[0:3]) == 227:
                msg = msg.split(' ')[-1]
                msg = list(eval(msg[0:-1]))
                data_port = msg[-2]*256 + msg[-1]
                if flag == 0:
                    break
                elif flag ==2:
                    msg_send = ('REST ' + str(segment[0]) + '\n').encode()
            elif int(msg[0:3]) == 530:
                sys.stderr.write("authentication failed")                
                exit(2)
            elif int(msg[0:3]) == 213:
                size =int(msg.split()[-9])
                msg_send = ('QUIT' + '\n').encode()
            elif int(msg[0:3]) == 221:
                sc_ctrl.close()
                return size
            elif int(msg[0:3]) == 350:
                break
            if log:                
               log.write('C->S: ' + msg_send.decode())   
            sc_ctrl.send(msg_send)     
        except:
            if msg and msg[0:3] == '530': exit(2)
            if size != -1: return size
            sys.stderr.write("generic error")
            exit(7)
    sc_data = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    try:
        sc_data.connect((ip_server,data_port))
    except:
        sys.stderr.write("can't connenct to server")
        exit(1)
    msg_send = ('RETR ' + file + '\n').encode()
    if log: log.write('C->S: ' + msg_send.decode())
    sc_ctrl.send(msg_send)
    while True:
        try:
            msg = sc_ctrl.recv(128).decode()   
            if log: log.write('S->C: ' + msg)
        except:            
            sys.stderr.write("generic error")
            exit(7)
        if int(msg[0:3]) == 150:
                break            
        elif int(msg[0:3]) == 550:
            sys.stderr.write("file not found")            
            exit(3)
    f = open(file,"br+")#why does br+ work???
    if segment:
        f.seek(segment[0])
        total_bytes = segment[1] - segment[0] + 1
    cur = 0
    while True:
        try:
            if flag == 2 and total_bytes - cur <=  1024:
                received_data = sc_data.recv(total_bytes-cur)  
                cur += total_bytes - cur    
                f.write(received_data)
                break
            received_data = sc_data.recv(1024)     
            # we know the total len is total_bytes; use cur to store bytes already received; 
            # then in the final receive: we know we only want total_bytes - cur bytes many data;
            # so ) : total - cur - 1; as 0: 2 are 2 bytes 
            if not received_data:
                break
            cur += len(received_data)
            f.write(received_data)
        except:
            sys.stderr.write("generic error")
            exit(7)
    f.close()
    print(cur)
    while True:
        try:
            msg = sc_ctrl.recv(128).decode()
            if log: log.write('S->C: ' + msg) 
            if int(msg[0:3]) == 226:
                msg_send = ('QUIT' + '\n').encode()
            elif int(msg[0:3]) == 221:
                sc_ctrl.close()
                sc_data.close()
                break
            if log: log.write('C->S: ' + msg_send.decode())
            sc_ctrl.send(msg_send)
        except:
            sys.stderr.write("generic error")
            exit(7)
           




'''
if 221: good
 2. DATA:
 connect to server node 
 receive data 

 send  to server: RETR file
 3. open a local file:
 write to 

 close DATA
 QUIT
'''

if __name__ == "__main__":
    ''' 
    argument parsing:
    -h/--help  (exits with 0) 
    -v/--version name,v,author,E:0 
    -f/--file 
    -s/--server hostname
    -p/--port defa21
    -n/--username defa:,
    -P/--password: defa:user@localhost.localnet
    -l/--log if file is "-":stdout, preprended by C->S,S->C
    -t/--thread para/config file, LINE FEED 0x0A ie \n
    '''
    parser = argparse.ArgumentParser(description = 'pftp project')
    parser.add_argument('-v','--version',action='version',version='pftp Yiyang Ou Ver:0.1')
    required = parser.add_argument_group("Required Arguments")
    required.add_argument('-f','--file',type=str,help='Specifies the file to download.',dest='file',required=True)
    required.add_argument('-s','--server',type=str,help='Specifies the server to download the file from.',dest='hostname',required=True)

    options = parser.add_argument_group("Options")    
    options.add_argument('-p','--port',type=int,help='Specifiefs the port to be used when contacting the server\
        .(default value:21)',dest='port',default=21)
    options.add_argument('-n','--username',type=str,help='Uses the username\
        user when logging into the FTP server (default value: anonymous).',dest='username',default='anonymous')
    options.add_argument('-P','--password',type=str,help='Uses the password\
        password when logging into the FTP server (default value:user@localhost.localnet).',dest='password',default='@localhost.localnet')
    options.add_argument('-l','--log',type=str,help='Logs all the FTP commands exchanged \
        with the server and the corresponding replies to file logfile. If the filename is \
        "-", then the commands are printed to the standard output.',dest='log',default=None)
    options.add_argument('-t','--thread',help='Specifies the config to use for paralell \
        download',type=str,dest='config_file',default=None)
    # behaves as -h if no parameter
    if len(sys.argv)==1:
        parser.print_help()
        exit(0)
    try:
        args =parser.parse_args()
    except:
        parser.exit(4)
    if args.password == '@localhost.localnet':
        args.password = args.username + '@localhost.localnet'
    port=args.port
    if args.log == '-':
        log = sys.stdout
    elif args.log is not None:
        log = open(args.log,'bw')
    else:
        log = None
    '''
    read from config file: args.config_file
    implement multithreading: read from server; log; write to file
    '''
    if args.config_file is not None:
        hostname_list = []
        username_list =[]
        password_list =[]
        file_path = []
        try:
            f = open(args.config_file)
        except:
            sys.stderr.write("config file not found")
            exit(4)
        lines = [line.rstrip('\n') for line in f]
        for conf in lines:
            try:
                username_list.append(conf.split('//')[-1].split(':')[0])
                password_list.append(conf.split('//')[-1].split(':')[1].split('@')[0])
                hostname_list.append(conf.split('@')[-1].split('/')[0])
                file_path.append(conf.split('@')[-1].split('/',2)[-1])
            except:
                sys.stderr.write("config file invalid")
                exit(4)
        f.close()        

        npartition = len(hostname_list)
        read_pos_list =[]
        size = read_from_server(args.hostname,args.username,args.file,args.password,1)
        size_per_partition = size // npartition
        for i in range(npartition-1):
            read_pos_list.append((i*size_per_partition,(i+1)*size_per_partition-1))
        read_pos_list.append(((npartition-1)*size_per_partition,size-1))
        th_list =[]
        open(args.file,'bw+')
        for i in range(npartition):
            th_list.append(threading.Thread(target=read_from_server,
                args=(hostname_list[i],
                    username_list[i],
                    args.file,
                    password_list[i],
                    2,
                    read_pos_list[i])))
            th_list[-1].start()
        for i in range(npartition):
            th_list[i].join()
    else:
        read_from_server(args.hostname,args.username,args.file,args.password)
    # print("file:%s, server: %s, port: %d, user: %s, pass: %s, log: %s" %(args.file,args.hostname,args.port,args.username,args.password,args.log))

    