LIBS=-licsc
OBJS=ping_sender.o
BIN=ping_sender
CC=gcc
CFLAGS=-I/usr/local/include
LDFLAGS=-I/usr/local/lib -pthread

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean:
	rm -f $(BIN) $(OBJS)
