# Makefile for drone

CC = gcc
OBJCS = drone8.c

CFLAGS =  -g -Wall
# setup for system
nLIBS =

all: drone8

drone8: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

clean:
	rm drone8