 #include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

int endfile = 0;
int fileindex = -1;
int lfr = -1;
char* buffer;
char* window;
int buffersize;
char* filename;

void *writeThread(void *vargp)
{
	FILE* fo = fopen(filename, "wb");
	srand (time(NULL));
	while(1) {
		if (endfile == 1 && lfr == fileindex) 
			break;
		if (fileindex < lfr) {
			//sleep(rand() % 1000);
			fileindex++;
			printf("Writing to file %d: %c\n", fileindex, buffer[fileindex % buffersize]);
			fwrite(&buffer[fileindex % buffersize], sizeof(char), 1, fo);
		}
		//sleep(10);
	}
	fclose(fo);
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc == 5) {
		filename = argv[1];
		int windowsize = strtol(argv[2], '\0', 10);
		buffersize = strtol(argv[3], '\0', 10);
		int port = strtol(argv[4], '\0', 10);
		
		int fd;
		char ack[7];
		char packet[9];
		buffer = (char*) malloc(sizeof(char)*buffersize);
		window = (char*) malloc(sizeof(char)*windowsize);
		
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror( "socket failed" );
			return 1;
		}
		printf("Open server port: %d\n",port);

		struct sockaddr_in serveraddr, clientaddr;
		memset( &serveraddr, 0, sizeof(serveraddr) );
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );
		
		unsigned int addrlength;
		int laf;
		
		FILE* fo = fopen(filename, "wb");

		if (bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
			perror("bind failed");
			return 1;
		}
		
		pthread_t tid;
		pthread_create(&tid, NULL, writeThread, NULL);
		
		while(1) {
			int length = recvfrom(fd, packet, 9, 0, (struct sockaddr*)&clientaddr, &addrlength);
			if (length < 0) {
				perror("recvfrom failed");
				break;
			}
			int packetnum;
			unsigned char data;
			memcpy(&packetnum, packet+1, sizeof(int));
			memcpy(&data, packet+6, sizeof(char));
			printf("Recieved data %d: %c\n", packetnum, data);
			
			if (packetnum == -1 && data == 0) {
				endfile = 1;
				printf("End of File\n");
				break;
			}
			
			laf = min(lfr + windowsize, fileindex + buffersize);
			if (packetnum <= laf && packetnum > lfr && window[packetnum % windowsize] == 0) {
				window[packetnum % windowsize] = 1;
				buffer[packetnum % buffersize] = data;
				while (window[(lfr+1) % windowsize] == 1) {
					window[(lfr+1) % windowsize] = 0;
					lfr++;
				}
			}
			memcpy(ack+1, &lfr, sizeof(int));
			printf("Send ACK %d\n", lfr);
			sendto(fd, ack, 7, 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr));
		}

		close(fd);
		pthread_join(tid, NULL);
	}
	else {
		printf("Enter arguments: <filename> <windowsize> <buffersize> <port_number>\n");
	}
	return 0;
}
