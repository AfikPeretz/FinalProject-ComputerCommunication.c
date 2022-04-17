# UDPPingClient.py
import time
from socket import *

clientName = '127.0.0.1'
clientPort = 1024
serverName = '127.0.0.1'
serverPort = 12000
sequence_number = 1
total_packets = 0
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.settimeout(1)
serverSocket.bind((clientName, clientPort))


def get_time():
  return int(round(time.time() * 1000))



def wait_for_response():
  global sequence_number
  while True:
    try:
      message, address = serverSocket.recvfrom(serverPort)
      return message
    except Exception:
      sequence_number = sequence_number + 1
      return ('Request timed out')

def send_message(message,wait=False):
   serverSocket.sendto(message.encode(), (serverName, serverPort))
   if wait == False:
     return
   else:
     return wait_for_response()



while sequence_number <= 10:
  message = ('PING ' + str(sequence_number) + ' ' + str(get_time()))
  recieved = send_message(message, True)
  if isinstance(recieved, str):
    print ('Request timed out')
    continue
  else:
    recieved_size = len(recieved)
    recieved_array = recieved.decode().split(' ')
    recieved_type = recieved_array[0].upper()
    recieved_seq = int(recieved_array[1])
    recieved_time = int(recieved_array[2])
    rtt = get_time() - recieved_time
  if rtt > 1000:
    continue
  if recieved_type == 'PING':
    print (str(recieved_size) + " bytes recieved from " + serverName + ':' + str(serverPort) + ': seq=' + str(recieved_seq) + ' rtt=' + str(rtt))
    sequence_number = sequence_number + 1
  elif recieved_type == 'ERROR':
    recieved_message = recieved_array[3]
    print (recieved)
  else:
    print ('There is an unknown problem.')
  last = recieved
  total_packets = total_packets + 1
