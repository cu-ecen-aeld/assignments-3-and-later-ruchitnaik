CC=gcc
CFLAGS=-I -g -Wall -Werror

all: writer

writer: writer.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) writer.c -o writer
	
exec = writer

.PHONY: clean
clean:
	rm -f $(exec) *.o
