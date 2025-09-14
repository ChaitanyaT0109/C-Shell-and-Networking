CC = gcc
CFLAGS = -Wall -Werror -g
DEPS = sham.h

all: client server

client: client.c $(DEPS)
	$(CC) $(CFLAGS) -o client client.c -lcrypto

server: server.c $(DEPS)
	$(CC) $(CFLAGS) -o server server.c -lcrypto

clean:
	rm -f client server *.o received_test_file.txt test_file.txt *.log
