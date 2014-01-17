#!/usr/bin/env python

"""
packet reader
"""

from scapy.all import *
import sys
import socket
import random
from time import sleep

if len(sys.argv) != 4:
	print "Usage: ", sys.argv[0], "<target address> <pcap> <client address in pcap to copy packets> "
	sys.exit()

target = sys.argv[1]
client = sys.argv[3]
pkts = rdpcap(sys.argv[2])

size = 1024

packetarray = []
port = pkts[0].payload.dport

def mutate(source, changerate=1):
	srclist = list(source)
	# print srclist
	for x in range(changerate):
		offset = random.randint(0,len(srclist)-1)
		srclist[offset] = struct.pack("B", random.randint(0,255))
	# print srclist
	return "".join(srclist)

for i in range(len(pkts)):
	payload = pkts[i].payload.payload.payload
	if str(payload) != '' and pkts[i].payload.src == client:
		packetarray.append(payload.load)

while True:
	pkt = random.choice(packetarray)
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		mutant = mutate(pkt)
		s.connect((target,port))
		s.send(mutant)
		print "Sent: %s" % repr(mutant)
		readable, writeable, exceptable = select([s],[],[], 0.1)
		if len(readable) != 0:
			recvData = s.recv(size)
			print "Received: %s" % repr(recvData)
		s.close()
	except IOError as e:
		if e.errno == 111:
			sys.stderr.write("Errnum: %d, Errmsg: %s\n" % (e.errno, e.strerror))
			break;
		pass
	# sleep(0.1)