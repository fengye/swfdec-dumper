CC=gcc
CFLAGS=-I/usr/local/include/swfdec-0.8/ -I/usr/local/include -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/cairo -Wall

LDFLAGS=-lcairo -lgio-2.0 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lswfdec-0.8 -lyaml

all: swfdec-dumper

swfdec-dumper: main.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o $@
clean:
	rm -rf *.o *.png swfdec-dumper


