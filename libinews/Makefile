CC = gcc-3.2
BASE_CFLAGS = -std=c99 -ggdb `pkg-config --cflags glib-2.0` `pkg-config --cflags gthread-2.0`
BASE_LDFLAGS = -ggdb -shared `pkg-config --libs glib-2.0` `pkg-config --libs gthread-2.0`

.c.o:
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -c $<

libinews.so.0: net.o utility.o init.o
	$(CC) $(BASE_LDFLAGS) $(LDFLAGS) -o libinews.so.0 utility.o net.o init.o

test: libinews.so.0 test.c
	$(CC) -ggdb -L. -linews -o test test.c

clean:
	rm -f *.o *.so *.so.0 test

all: libinews.so.0
