all: crc server client

server: server.c
	gcc -o server server.c

client: client.c
	gcc -o client client.c

crc: crc.c
	gcc -o crc crc.c
