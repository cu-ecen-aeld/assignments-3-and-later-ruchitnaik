#native compiler
CC=gcc

#compilation flags
CFLAGS=-I -g -Wall -Werror

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
			$(CROSS_COMPILE)$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket -pthread
			
exec=aesdsocket

.PHONY: clean
clean:
		rm -f $(exec) *.o
