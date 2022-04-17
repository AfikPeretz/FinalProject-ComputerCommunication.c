# UDPHeartBeatClient.py
from socket import *
import time
import random

# What's your IP address and witch port should we use?
clientName = '127.0.0.1'
clientPort = 1024
serverName = '127.0.0.1'
serverPort = 12000

sequence_number = 1
minRTT = 0
maxRTT = 0
avgRTT = 0
packets_dropped = 0
total_packets = 0


# Setup a UDP socket
# Notice the use of SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)
# Assign IP address and port number to socket
serverSocket.settimeout(1)
serverSocket.bind((clientName, clientPort))

def get_time():
  return int(round(time.time() * 1000))


def wait_for_response():
  global packets_dropped
  global sequence_number
  global total_packets
  while True:
    # Generate random number in the range of 0 to 10 rand = random.randint(0, 10)
    # Receive the client packet along with the address it is coming from
    try:
      message, address = serverSocket.recvfrom(serverPort)
      return message
    except Exception:
      total_packets = total_packets 
      packets_dropped = packets_dropped +1
      sequence_number = sequence_number + 1
      return 'Request timed out'
      


def send_message(message,wait=False):
   serverSocket.sendto(message.encode(), (serverName, serverPort))
   if wait == False:
     return
   else:
     return wait_for_response()

while sequence_number < 11:
  time.sleep(1)
  # Create message with current sequence_number and time
  message = 'PING ' + str(sequence_number) + ' ' + str(get_time())
  # Recieve ping
  recieved = send_message(message, True)
  if isinstance(recieved, str):
    print ('Request timed out')
    continue
  else:
    recieved_size = len(recieved)
    recieved_array = recieved.decode().split(' ')
    recieved_type = recieved_array[0].upper()
    #print recieved_type
    recieved_seq = recieved_array[1]
    recieved_time = int(recieved_array[2])
    #print recieved_time
    rtt = get_time() - recieved_time
  if rtt > 1000:
    continue
  if recieved_type == 'PING':
    print (str(recieved_size) + " bytes recieved from " + serverName + ':' + str(serverPort) + ': seq=' + str(recieved_seq) + ' rtt=' + str(rtt))
    avgRTT = avgRTT + rtt
    if rtt < minRTT or minRTT == 0:
      minRTT = rtt
    if rtt > maxRTT or maxRTT== 0:
      maxRTT = rtt
    sequence_number = sequence_number + 1
  elif recieved_type == 'ERROR':
    recieved_message = recieved_array[3]
    print (recieved)
    if recieved_message == 'TIMEOUT':
      sequence_number = 1
      print ('Timeout: waiting 5 seconds before reconnect')
      time.sleep(5)
  else:
    print ('Something went wrong, but I have no idea what it is.')
  last = recieved

  total_packets = total_packets + 1
# Out of the loop, report running statistics
print (("RTT: min=") + str(minRTT) + " max=" + str(maxRTT) + " avg=" + str(avgRTT/(10-packets_dropped)))
print (("Packet Loss: ") + str(packets_dropped/(total_packets+packets_dropped)*100) + "%")
