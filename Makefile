CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -std=gnu99

client: client.c
	$(CC) -o client client.c

server: server.c
	$(CC) -o server server.c

runc: client
	./client "127.0.0.1" "4444"

runs: server
	./server "prog.job" "4444"

checkc: client
	valgrind --leak-check=full --show-leak-kinds=all ./client "127.0.0.1" "4444"

checks: server
	valgrind --leak-check=full --show-leak-kinds=all ./server "prog.job" "4444"
