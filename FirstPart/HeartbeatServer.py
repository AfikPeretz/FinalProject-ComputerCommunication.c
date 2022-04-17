# UDPHeartBeatServer.py
# We will need the following module to generate randomizedlost packets
from socket import *
import time
import random

serverName = '127.0.0.1'
serverPort = 12000
clientName = '127.0.0.1'
clientPort = 1024

# Create a UDP socket
# Notice the use of SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)
# Assign IP address and port number to socket
serverSocket.bind((serverName, serverPort))

simulate_packet_loss = True
sleep_for_rand_response_times = True

sequence_number = 0
recieved_time = 0

def get_time():
  return int(round(time.time() * 1000))

def send_message(message,wait=False):
   serverSocket.sendto(message, (clientName, clientPort))
   if wait == False:
     return
   else:
     return wait_for_response()

# Just respond to ping requests
while True:
  # Receive the client packet along with the address it is coming from
  message, address = serverSocket.recvfrom(clientPort)
  print (get_time() - recieved_time)
  if recieved_time != 0 and get_time() - recieved_time > 5000:
    print ('Client disconnect (timeout)')
    recieved_time = 0
  # Capitalize the message from the client
  message = message.upper()
  recieved_size = len(message)
  recieved_array = message.decode().split(' ')
  recieved_type = recieved_array[0].upper()
  #print recieved_type
  recieved_seq = int(recieved_array[1])
  recieved_time = int(recieved_array[2])
  if recieved_seq != sequence_number+1:
    if sequence_number != 0:
      for i in range(sequence_number, recieved_seq):
        print ('Dropped Packet:' + str(i))
    if sequence_number == 0:
      print ('Client connect.')
    sequence_number = recieved_seq
  print ('Recieve: ' + message.decode())
  # If rand is less is than 4, we consider the packet lost and do not respond
  if sleep_for_rand_response_times:
    min_sleep = 0.2
    max_sleep = 1.0
    time.sleep(random.uniform(min_sleep, max_sleep))
    if simulate_packet_loss:
      if random.randint(1, 10) < 4:
        print ('Request timed out')
        continue
    
  serverSocket.sendto(message, address)
  print ('Send: ' + message.decode())