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
import sys
port = 21
log = ''



def read_from_server(server,username,file,password):
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
    while True:
        try:
            # remeber to write log and error handler (such as host name incorrect blahblah)
            msg = sc_ctrl.recv(128).decode()
            print(msg)
            if int(msg[0:3]) == 220:
                sc_ctrl.send(('USER ' + username + '\n').encode())
            elif int(msg[0:3]) == 331:
                sc_ctrl.send(('PASS ' + password + '\n').encode())
            elif int(msg[0:3]) == 230:
                sc_ctrl.send(('TYPE I' + '\n').encode())
            elif int(msg[0:3]) == 200:
                sc_ctrl.send(('PASV ' + username + '\n').encode())
            elif int(msg[0:3]) == 227:
                break
            elif int(msg[0:3]) == 530:
                sys.stderr.write("authentication failed")
                exit(2)
        except:
            if msg: exit(2)
            sys.stderr.write("generic error")
            exit(7)
    msg = msg.split(' ')[-1]
    msg = list(eval(msg[0:-1]))
    data_port = msg[-2]*256 + msg[-1]
    sc_data = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    try:
        sc_data.connect((ip_server,data_port))
    except:
        sys.stderr.write("can't connenct to server")
        exit(1)
    f = open(file,"bw+")
    sc_ctrl.send(('RETR ' + file + '\n').encode())
    while True:
        try:
            msg = sc_ctrl.recv(128).decode()    
        except:            
            sys.stderr.write("generic error")
            exit(7)
        if int(msg[0:3]) == 150:
            break
        elif int(msg[0:3]) == 550:
            sys.stderr.write("file not found")            
            exit(3)
    while True:
        try:
            received_data = sc_data.recv(1024)            
            if not received_data:
                break  
            f.write(received_data)
        except:
            sys.stderr.write("generic error")
            exit(7)
    while True:
        try:
            msg = sc_ctrl.recv(128).decode()
            if int(msg[0:3]) == 226:
                sc_ctrl.send(('QUIT' + '\n').encode())
            elif int(msg[0:3]) == 221:
                break
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
    required.add_argument('-f','--file',type=str,dest='file',required=True)
    required.add_argument('-s','--server',type=str,dest='server',required=True)

    options = parser.add_argument_group("Options")    
    options.add_argument('-p','--port',type=int,dest='port',default=21)
    options.add_argument('-n','--username',type=str,dest='username',default='anonymous')
    options.add_argument('-P','--password',type=str,dest='password',default='@localhost.localnet')
    options.add_argument('-t','--thread',type=str,dest='config_file',default='')
    options.add_argument('-l','--log',type=str,dest='log',default='')
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
    port,log=args.port,args.log
    read_from_server(args.server,args.username,args.file,args.password)
    # print("file:%s, server: %s, port: %d, user: %s, pass: %s, log: %s" %(args.file,args.hostname,args.port,args.username,args.password,args.log))

    