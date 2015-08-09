CC = gcc
CFLAGS = -Wall
TARGETS = serwer 

all: $(TARGETS) 

serwer: serwer.o err.o err.h
	$(CC) $(CFLAGS) $^ -o $@ -levent

clean:
	rm -f *.o $(TARGETS) 
