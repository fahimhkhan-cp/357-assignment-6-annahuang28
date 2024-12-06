CC = gcc
CFLAGS = -Wall -std=c99 -pedantic
PROCESS = process
PROCESS_OBJS = process.o
PROGS = $(PROCESS)

# Default target - build the process executable
all: $(PROGS)

# Rule to compile the process executable from process.c
$(PROCESS): $(PROCESS_OBJS)
	$(CC) $(CFLAGS) -o $(PROCESS) $(PROCESS_OBJS)

process.o : process.c
	$(CC) $(CFLAGS) -c process.c

clean :
	rm *.o $(PROGS) core

