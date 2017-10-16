all: server client

server: server.c
	gcc -pthread -o server server.c

client: client.c crc.c crc.h
	gcc -o client client.c crc.c
