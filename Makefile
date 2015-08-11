CC = gcc
CFLAGS = -Wall
TARGETS = opoznienia 

all: $(TARGETS) 

opoznienia: opoznienia.o err.o err.h args.o args.h mdns.o mdns.h
	$(CC) $(CFLAGS) $^ -o $@ -levent

clean:
	rm -f *.o $(TARGETS) 
