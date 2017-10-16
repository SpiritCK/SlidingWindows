all: server client

server: server.c crc.c crc.h
	gcc -pthread -o server server.c crc.c

client: client.c crc.c crc.h
	gcc -o client client.c crc.c
