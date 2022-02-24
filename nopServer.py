#!/usr/bin/python

# this is only used for testing
# once the server is booted, it will spin forever

import socket
import sys

serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serverSocket.bind(('', int(sys.argv[1])))
serverSocket.listen(5)

while 1:
    channel, details = serverSocket.accept()
    print ('Connection Establish with Nop Server' +  str(details))

    while 1: # spin forever
        continue