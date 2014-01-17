#!/usr/bin/env python

"""
Simple blackbox client
"""

import socket
import sys
import random
import binascii
import math
from struct import *
from time import sleep

if len(sys.argv) != 2:
	print "Usage: ./client.py <server address>"
	sys.exit()
host = sys.argv[1]

port = 5000
size = 1024

def doNothing():
	print "Error, that is an unknown type"

def reqTime():
	sendbuf = pack('!1B', 1);
	print "sending:",binascii.hexlify(sendbuf);
	s.send(sendbuf)
	data = s.recv(size)
	print 'Received:', data

def randomString(length):
	chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
	badchars = "%[]{}\"'.\\/?!@#$^&*()"
	badgroups = ['%n','%x']
	retType = random.randint(0,100)
	if(retType < 80 or length < 2):
		return ''.join(random.choice(chars) for i in range(length))
	else:
		print "-----mean"
		return ''.join(random.choice(badgroups)).join(random.choice(chars+badchars) for i in range(length-2))

def pointInt(sendbuf):
	pointID = random.randint(1,20);
	pointValue = random.randint(1,math.pow(2,31))
	sendbuf += pack('!BLL', 1, pointID, pointValue)
	print "sending:",binascii.hexlify(sendbuf);
	s.send(sendbuf)
	data = s.recv(size)
	print 'Received:', data
	return

def pointMsg(sendbuf):
	rndtxt = randomString(random.randint(1,10))
	# rndtxt = "%n%n%n"
	checksum = 0
	for char in rndtxt:
		checksum += ord(char)
	checksum = 0xff - (checksum & 0xff) + 1
	sendbuf += pack('!BL%dsB' % len(rndtxt), 2, len(rndtxt),rndtxt, checksum);
	print "sending:",binascii.hexlify(sendbuf);
	s.send(sendbuf)
	data = s.recv(size)
	print 'Received:', data

def pointBlob(sendbuf):
	blob = ""
	flags = 0
	if random.randint(0,100) < 50:
		flags = random.randint(1,255);
	for i in range(10):
		blob += pack('B', random.randint(0,255))
	sendbuf += pack('!BB%dp' % len(blob), 3, flags, blob);
	print "sending:",binascii.hexlify(sendbuf);
	s.send(sendbuf)
	data = s.recv(size)
	print 'Received:', data

def getPointType(pointType):
	return {
		1:pointInt,
		2:pointMsg,
		3:pointBlob
	}.get(pointType,doNothing) #donothing default case

def reqType2():
	pointType = random.randint(1,3);
	print "** Point %d" % pointType
	sendbuf = pack('!B', 2);
	getPointType(pointType)(sendbuf)

def getMethod(method):
	return {
		1:reqTime,
		2:reqType2
	}.get(method,doNothing) #donothing default case


# s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# s.connect((host,port))
# buf = pack('!B', 2);
# pointMsg(buf);

while True:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((host,port))

	reqTime()
	for i in range(3):
		s.close()
		sleep(1)
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.connect((host,port))
		reqType2()
	s.close()