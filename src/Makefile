CC=gcc
#CFLAGS=-g -O0 -Wall #-DNDEBUG
CFLAGS=-DNDEBUG -Wall

framesize ?= 18
varmemsize ?= 10

MEMFLAGS = -DFRAME_STORE_SIZE=$(framesize) -DVAR_MEM_SIZE=$(varmemsize)
SRCS = shell.c interpreter.c shellmemory.c pcb.c queue.c schedule_policy.c
OBJS = $(SRCS:.c=.o)

mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -c $(SRCS)
	$(CC) $(CFLAGS) -o $(OBJS)

clean: 
	rm mysh; rm *.o
