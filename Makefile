.POSIX:
.SUFFIXES:
CC = cc
CFLAGS = -W -O
LDLIBS = -lusb

all: s1display
s1display: log.o main.o lcd_device.o
	$(CC) $(LDFLAGS) -o s1display log.o lcd_device.o  main.o $(LDLIBS)
log.o: log.c log.h
lcd_device.o: lcd_device.c lcd_device.h
main.o: main.c
clean:
	rm s1display log.o lcd_device.o main.o

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: all clean
