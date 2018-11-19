#!/usr/bin/env python3

import sys
import threading



def helper(i):
    f = open('a.txt','w+')
    f.seek(i)
    for k in range(5):
        f.write('2')
    f.close()
f = open('a.txt','w+')
for i in range(10):
    f.write('1')
f.close()

th1 = threading.Thread(target=helper,args=(0,))
th2 = threading.Thread(target=helper,args=(5,))
th1.start()
th2.start()
th1.join()
th2.join()