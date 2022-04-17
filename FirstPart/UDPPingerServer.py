from socket import *
import time
import random

serverName = '127.0.0.1'
serverPort = 12000
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind((serverName, serverPort))
recieved_time = 0


# Just respond to ping requests
while True:
  # Receive the client packet along with the address it is coming from
  message, address = serverSocket.recvfrom(1024)
  # Capitalize the message from the client
  message = message.upper()
  recieved_array = message.decode().split(' ')
  recieved_type = recieved_array[0].upper()
  #print recieved_type
  recieved_seq = int(recieved_array[1])
  recieved_time = int(recieved_array[2])
  min_sleep = 0.2
  max_sleep = 1.0
  time.sleep(random.uniform(min_sleep, max_sleep))
  # If rand is less is than 4, we consider the packet lost and do not respond
  if random.randint(0, 10) < 4:
    	print ('Request timed out')
    	continue
  serverSocket.sendto(message, address)
  print ('Send: ' + message.decode())