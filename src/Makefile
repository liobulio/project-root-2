#CC=gcc
#CFLAGS=-pthread # c compiler flags	
#FMT=indent

#mysh: shell.c interpreter.c shellmemory.c pcb.c ready_queue.c scheduler.c
#	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c pcb.c ready_queue.c scheduler.c
#	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o ready_queue.o scheduler.o

#style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h pcb.c pcb.h ready_queue.c ready_queue.h scheduler.c scheduler.h
#	$(FMT) $?

#clean: 
#	$(RM) mysh; $(RM) *.o; $(RM) *~


CC=gcc
#CFLAGS=-g -O0 -Wall #-DNDEBUG
CFLAGS=-DNDEBUG -Wall

framesize ?= 18
varmemsize ?= 10

MEMFLAGS = -DFRAME_STORE_SIZE=$(framesize) -DVAR_MEM_SIZE=$(varmemsize)
SRCS = shell.c interpreter.c shellmemory.c pcb.c ready_queue.c scheduler.c
OBJS = $(SRCS:.c=.o)							# change all suffix from .c to .o

mysh: $(OBJS)
	$(CC) $(CFLAGS) -o mysh $(OBJS)

%.o: %.c 										# compile any .c to .o
	$(CC) $(CFLAGS) $(MEMFLAGS) -c $< -o $@		# $< are source(.c) and $@ are destiantion(.o)

clean: 
	rm mysh; rm *.o

