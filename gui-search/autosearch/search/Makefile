TARGET= search

CSOURCE= search.c util.c
A_SRC  = $(wildcard *-a.c)
HEADER = tokenizer.h index.h sorted-list.h util.h

OBJ   = $(CSOURCE:=.o)
A_OBJ = $(A_SRC:=.o) $(OBJ)
B_OBJ = $(B_SRC:=.o) $(OBJ)
TRASH = gmon.out

ALL_CFLAGS = -ansi -Wall $(CFLAGS)
ALL_LDFLAGS= $(ALL_CFLAGS) $(LDFLAGS) 

CC = gcc
LD = gcc
RM = rm -f

all: $(TARGET)

$(TARGET) : $(A_OBJ) 
	$(LD) $(ALL_LDFLAGS) -o $@ $^
clean:
	$(RM) $(TARGET) $(TRASH) $(A_OBJ)

%.c.o : %.c $(HEADER)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

.PHONY: all clean
