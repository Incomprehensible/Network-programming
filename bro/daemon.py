#!/usr/bin/env python3

import os
import sys

r, w = map(int, sys.argv[1:])
f = open("kek", 'w')
# f.write("This is test")
# f.write(str(r))
# f.write(str(w))

while True:
    response = ''
    while len(response) < 2:
        response = os.read(r, 2)
        # f.write('response is: ')
        # f.write(str(response))
    f.write('got response\n')
    if response == b'21':
        res = encoding.start()
        f.write('Encoding was envoked...\n')
        os.write(w, b'0')
        continue
    elif response == b'42':
        f.write("Recognizing was envoked...\n")
        res = recognizing.start()
        os.write(w, b'1')
        continue
    else:
        f.write("nothing familiar...\n")

f.write('Quiting...\n')
f.close()
quit()