CC = gcc
CFLAGS = -Wall
TARGETS = main 

all: $(TARGETS) 

main: main.o err.o err.h args.o args.h mdns.o mdns.h socket_event_manager.o socket_event_manager.h util.o util.h delays.o delays.h
	$(CC) $(CFLAGS) $^ -o $@ -levent

clean:
	rm -f *.o $(TARGETS) 
