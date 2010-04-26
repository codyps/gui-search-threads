TARGET = thread-test

SRC = $(wildcard *.c) 
HEADER = $(wildcard *.h)

OBJ   = $(SRC:=.o)
TRASH = gmon.out

ALL_CFLAGS = -Wall $(CFLAGS) -pthreads
ALL_LDFLAGS= $(ALL_CFLAGS) $(LDFLAGS)

CC = gcc
LD = gcc
RM = rm -f

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(LD) $(ALL_LDFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET) $(TRASH) $(OBJ)

%.c.o : %.c $(HEADER)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

.PHONY: all clean
