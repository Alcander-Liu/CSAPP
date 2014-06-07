#
# Students' Makefile for the Malloc Lab
#

CC = gcc
CFLAGS = -Wall -O2 -m32 -pg -rdynamic -L/usr/local/lib/ -I/usr/local/include

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

test: unit_test.o memlib.o mm.o
	$(CC) $(CFLAGS) -o unit_test unit_test.o memlib.o mm.o -lcheck

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h
unit_test.o: unit_test.c mm.h

handin:
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	rm -f *~ *.o mdriver


