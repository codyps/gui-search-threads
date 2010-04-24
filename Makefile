##############
## Makefile ##
##############
TARGET = gui-search

SRC = $(wildcard *.c) 
HEADER = $(wildcard *.h)

OBJ   = $(SRC:=.o)
TRASH = gmon.out

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-2.0`
ALL_CFLAGS = -Wall $(CFLAGS) $(GTK_CFLAGS)
ALL_LDFLAGS= $(ALL_CFLAGS) $(LDFLAGS) $(GTK_LDFLAGS)

CC = gcc
LD = gcc
RM = rm -f

all: $(TARGET) helpers

helpers :
	$(MAKE) -C autosearch

$(TARGET) : $(OBJ)
	$(LD) $(ALL_LDFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET) $(TRASH) $(OBJ)
	$(MAKE) -C autosearch clean

%.c.o : %.c $(HEADER)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

.PHONY: all clean
