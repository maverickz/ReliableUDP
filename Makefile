# Makefile for client
CC = gcc
CFLAGS = -lxnet
FTPCLI = ftpclient.c 
TCPDCLIENT = tcpdclient.c
FTPSERV = ftpserver.c
TIMER = timer.c
TCPDSERVER = tcpdserver.c


all: ftpc tcpdc ftps tcpds timer

ftpc:	$(FTPCLI)
	$(CC) $(CFLAGS) -o $@ $(FTPCLI) 

tcpdc:  $(TCPDCLIENT)
	$(CC) $(CFLAGS) -o $@ $(TCPDCLIENT) 

ftps:   $(FTPSERV)
	$(CC) $(CFLAGS) -o $@ $(FTPSERV) 

tcpds:  $(TCPDSERVER)
	$(CC) $(CFLAGS) -o $@ $(TCPDSERVER) 

timer:	$(TIMER)
	$(CC) $(CFLAGS) -o $@ $(TIMER) 

clean:
	rm ftpc tcpdc tcpds ftps timer
