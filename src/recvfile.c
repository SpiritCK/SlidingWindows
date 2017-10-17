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
#include "crc.h"

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

int endfile = 0;
int fileindex = -1;
int lfr = -1;
unsigned char* buffer;
unsigned char* window;
int buffersize;
char* filename;
int sleepdelay = 100; //(ms)
FILE* fl;
char* ip_addr;
char logmsg[500];

void Log(char* tag, char* msg) {
	time_t ltime = time(NULL);
	struct tm result;
	char stime[32];
	localtime_r(&ltime, &result);
	asctime_r(&result, stime);
	stime[strlen(stime)-1] = 0;
	if (tag == NULL) {
		fprintf(fl, "%s -- [%s] [note] %s\n", ip_addr, stime, msg);
	}
	else {
		fprintf(fl, "%s -- [%s] [%s] %s\n", ip_addr, stime, tag, msg);
	}
}

void *writeThread(void *vargp)
{
	FILE* fo = fopen(filename, "wb");
	srand (time(NULL));
	while(1) {
		if (endfile == 1 && lfr == fileindex) 
			break;
		if (fileindex < lfr) {
			usleep((rand() % sleepdelay) * 1000);
			printf("Writing to file %d: 0x%02x\n", fileindex+1, buffer[(fileindex+1) % buffersize]);
			fwrite(&buffer[(fileindex+1) % buffersize], sizeof(char), 1, fo);
			sprintf(logmsg, "Reading from buffer byte %d (index %d): 0x%02x", fileindex+1, (fileindex+1) % buffersize, (buffer[(fileindex+1) % buffersize])&0xFF);
			Log("buffer", logmsg);
			fileindex++;
		}
		usleep(1000);
	}
	fclose(fo);
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc == 5) {
		fl = fopen("log/recvlog.txt", "wb");

		filename = argv[1];
		int windowsize = strtol(argv[2], '\0', 10);
		buffersize = strtol(argv[3], '\0', 10);
		int port = strtol(argv[4], '\0', 10);
		ip_addr = "(server)";
		
		sprintf(logmsg, "Recieving %s with windowsize %d and buffersize %d", filename, windowsize, buffersize);
		Log("init", logmsg);
		
		int fd;
		unsigned char ack[7];
		unsigned char packet[9];
		buffer = (char*) malloc(sizeof(char)*buffersize);
		window = (char*) malloc(sizeof(char)*windowsize);
		for (int i = 0; i < windowsize; i++) {
			window[i] = 0;
		}
		
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror( "socket failed" );
			return 1;
		}

		struct sockaddr_in serveraddr, clientaddr;
		memset( &serveraddr, 0, sizeof(serveraddr) );
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		unsigned int addrlength;
		int laf;
		
		FILE* fo = fopen(filename, "wb");

		if (bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
			perror("bind failed");
			return 1;
		}
		printf("Open server port: %d\n",port);
		sprintf(logmsg, "Open port %d", port);
		Log("init", logmsg);
		
		pthread_t tid;
		pthread_create(&tid, NULL, writeThread, NULL);
		
		while(1) {
			int length = recvfrom(fd, packet, 9, 0, (struct sockaddr*)&clientaddr, &addrlength);
			if (length < 0) {
				perror("recvfrom failed");
				break;
			}
			ip_addr = inet_ntoa(clientaddr.sin_addr);
			int packetnum;
			unsigned char data;
			
			if (decryptCRC(packet, 9) == 1) {
				memcpy(&packetnum, packet+1, sizeof(int));
				memcpy(&data, packet+6, sizeof(char));
				printf("Recieved data %d: 0x%02x\n", packetnum, data);
			
				if (packetnum == -1 && data == 0) {
					endfile = 1;
					printf("End of File\n");
					Log("packet", "EOF packet Recieved");
					break;
				}
				sprintf(logmsg, "Packet %d recieved with data: 0x%02x", packetnum, data);
				Log("packet", logmsg);
				
				laf = min(lfr + windowsize, fileindex + buffersize);
				if (packetnum <= laf && packetnum > lfr && window[packetnum % windowsize] == 0) {
					window[packetnum % windowsize] = 1;
					buffer[packetnum % buffersize] = data;
					sprintf(logmsg, "Writing to buffer byte %d (index %d): 0x%02x", packetnum, packetnum % buffersize, data);
					Log("buffer", logmsg);
					int templfr = lfr;
					printf("Check lfr: %d\n", lfr);
					while (window[(lfr+1) % windowsize] == 1) {
						window[(lfr+1) % windowsize] = 0;
						lfr++;
					}
					if (lfr != templfr) {
						sprintf(logmsg, "Moving window from %d to %d", templfr, lfr);
						Log("window", logmsg);
					}
				}
				
				ack[0] = 0x6;
				memcpy(ack+1, &lfr, sizeof(int));
				ack[5] = (char) (buffersize - (lfr - fileindex));
				ack[6] = encryptCRC(ack, 6);
				
				printf("Send ACK %d\n", lfr);
				sendto(fd, ack, 7, 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr));
				sprintf(logmsg, "Sending ACK %d with AWS: %d", lfr, ack[5]);
				Log("ACK", logmsg);
			}
			else {
				printf("Corrupted packet recieved\n");
				Log("WARN", "Recieved corrupted packet");
			}
			printf("\n");
		}

		fclose(fl);
		close(fd);
		pthread_join(tid, NULL);
	}
	else {
		printf("Enter arguments: <filename> <windowsize> <buffersize> <port_number>\n");
	}
	return 0;
}
