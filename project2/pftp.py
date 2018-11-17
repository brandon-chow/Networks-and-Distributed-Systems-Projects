#!/usr/bin/env python3

'''
1. parse argument / from config
2. how protocles work with socket interface
3. how to retrieve from server; create a local object
5. username/ password?
6. log
7. paralell 
8. error

'''
import argparse




if __name__ == "__main__":
	''' 
	argument parsing:
	-h/--help  (exits with 0) 
	-v/--version name,v,author,E:0 
	-f/--file 
	-s/server hostname
	-p/--port
	-n/--username defa:21,
	-P/--password: defa:user@localhost.localnet
	-l/--log if file is "-":stdout, preprended by C->S,S->C
	-t/--thread para/config file, LINE FEED 0x0A ie \n
	'''
	parser = argparse.ArgumentParser(description = 'pftp project')
	parser.add_argument('-v','--version',action='version',version='pftp Yiyang Ou Ver:0.1')
	parser.add_argument('file',type=str,