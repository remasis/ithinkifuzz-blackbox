#!/usr/bin/env python

"""
packet reader
"""

from scapy.all import *

pkts = rdpcap(sys.argv[1])

lastwrite = 0
for i in range(len(pkts)):
	payload = pkts[i].payload.payload.payload
	if str(payload) != '':
		if lastwrite != i-2:
			print("####")
		lastwrite = i
		print "#" , pkts[i].payload.src
		print repr(str(payload))