.POSIX:
.SUFFIXES:

# Compiler settings
CC = cc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -O2
CPPFLAGS = -D_DEFAULT_SOURCE
LDFLAGS =
LDLIBS = -lusb

# Project files
SRCS = main.c log.c lcd_device.c
OBJS = $(SRCS:.c=.o)
TARGET = s1display

# Targets
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# Dependencies
main.o: main.c log.h lcd_device.h
log.o: log.c log.h
lcd_device.o: lcd_device.c lcd_device.h log.h

# Generic rules
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
