TARGET = thread-test

SRC = errors.c threadpool.c
HEADER = $(wildcard *.h)

SRCA = $(SRC) threadpool_test.c
SRCB = $(SRC) example_thread.c

OBJA  = $(SRCA:=.o)
OBJB  = $(SRCB:=.o)
TRASH = gmon.out

CFLAGS= -g
ALL_CFLAGS = -Wall $(CFLAGS) -pthread
ALL_LDFLAGS= $(ALL_CFLAGS) $(LDFLAGS)

CC = gcc
LD = gcc
RM = rm -f

all: $(TARGET)-a $(TARGET)-b

$(TARGET)-b : $(OBJB)
	$(LD) $(ALL_LDFLAGS) -o $@ $^

$(TARGET)-a : $(OBJA)
	$(LD) $(ALL_LDFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET) $(TRASH) $(OBJA) $(OBJB)

%.c.o : %.c $(HEADER)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

.PHONY: all clean
