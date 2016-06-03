LIBS=$(shell pkg-config --libs icsc)
OBJS=ping_sender.o
BIN=ping_sender
CC=gcc
CFLAGS=$(shell pkg-config --cflags icsc)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean:
	rm -f $(BIN) $(OBJS)
