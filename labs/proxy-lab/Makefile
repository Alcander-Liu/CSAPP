TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L7/handin

CC = clang
CFLAGS = -Wall -g -std=c99
LDFLAGS = -lpthread

OBJS = proxy.o xnix_helper.o

all: proxy

proxy: $(OBJS)

# csapp.o: csapp.c
# 	$(CC) $(CFLAGS) -c csapp.c

xnix_helper.o: xnix_helper.c xnix_helper.h
	$(CC) $(CFLAGS) -c xnix_helper.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c

clean:
	rm -f *~ *.o proxy core

