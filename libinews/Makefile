CC = gcc-3.2
LN = ln
BASE_CFLAGS = -std=c99 -fPIC -ggdb -D_REENTRANT
BASE_LDFLAGS = -ggdb -fPIC -shared -lpthread

.c.o:
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -c $<

libinews.so.0: net.o utility.o init.o IList.o
	$(CC) $(BASE_LDFLAGS) $(LDFLAGS) -o libinews.so.0 IList.o utility.o net.o init.o
	$(LN) -sf libinews.so.0 libinews.so

test: libinews.so.0 test.c
	$(CC) -ggdb -L. -linews -o test test.c

clean:
	rm -f *.o *.so *.so.0 test

all: libinews.so.0
