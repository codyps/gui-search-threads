TARGET= search

SOURCE = $(wildcard *.c)
HEADER = tokenizer.h index.h sorted-list.h util.h

OBJ   = $(SOURCE:=.o)
TRASH = gmon.out

CFLAGS = -g
ALL_CFLAGS = -Wall -pthread -lm $(CFLAGS)
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
