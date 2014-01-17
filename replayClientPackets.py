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

for i in range(len(pkts)):
	payload = pkts[i].payload.payload.payload
	if str(payload) != '' and pkts[i].payload.src == client:
		packetarray.append(payload.load)

while True:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((target,port))
	pkt = random.choice(packetarray)
	print 'Sending: ', repr(pkt)
	s.send(pkt)
	rcvData = s.recv(size)
	print 'Received: ', rcvData
	sleep(0.2)