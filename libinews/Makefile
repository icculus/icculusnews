net.o: net.c
	gcc-3.2 -ggdb `pkg-config --cflags glib-2.0` `pkg-config --cflags gthread-2.0` -c -o net.o net.c
utility.o: utility.c
	gcc-3.2 -ggdb `pkg-config --cflags glib-2.0` `pkg-config --cflags gthread-2.0` -c -o utility.o utility.c
init.o: init.c
	gcc-3.2 -ggdb `pkg-config --cflags glib-2.0` `pkg-config --cflags gthread-2.0` -c -o init.o init.c
libinews.so.0: net.o utility.o init.o
	gcc-3.2 -ggdb -shared `pkg-config --libs glib-2.0` `pkg-config --libs gthread-2.0` -o libinews.so.0 utility.o net.o init.o

test: libinews.so.0 test.c
	gcc-3.2 -ggdb -L. -linews -o test test.c

clean:
	rm -f *.o *.so *.so.0 test

all: libinews.so.0 test
