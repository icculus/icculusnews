CC = gcc-3.2
LN = ln
BASE_CFLAGS = -std=c99 -ggdb -fPIC -DPIC -D_REENTRANT
BASE_LDFLAGS = -ggdb -fPIC -DPIC -shared -lpthread

.c.o:
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -c $<

libinews.so.0: net.o utility.o init.o IList.o
	$(CC) $(BASE_LDFLAGS) $(LDFLAGS) -o libinews.so.0 IList.o utility.o net.o init.o
	$(LN) -sf libinews.so.0 libinews.so

libinews.a: net.o utility.o init.o IList.o
	$(AR) cru libinews.a net.o utility.o init.o IList.o

test: libinews.so.0 test.c
	$(CC) -ggdb -L. -linews -o test test.c

test-static: libinews.a test.c
	$(CC) -ggdb -o test.o -c test.c
	$(CC) -lpthread -ggdb $(LDFLAGS) -o test-static test.o libinews.a

clean:
	rm -f *.o *.so *.a *.so.0 test test-static

all: libinews.so.0 libinews.a
