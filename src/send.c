 #include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "crc.h"

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

int main(int argc, char *argv[])
{
	if (argc == 6) {
		char* filename = argv[1];
		int windowsize = strtol(argv[2], '\0', 10);
		int buffersize = strtol(argv[3], '\0', 10);
		char* ip_addr = argv[4];
		int port = strtol(argv[5], '\0', 10);
		
		int fd;
		unsigned char ack[7];
		unsigned char packet[9];
		char end[1] = {0};
		char buffer[buffersize];
		
		FILE* fi;
		fi = fopen(filename, "rb");
		if (fi == NULL) {
			printf("File not found\n");
			exit(1);
		}
		
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
			perror("Socket failed");
			return 1;
		}
		printf("Port: %d\n",port);
		printf("Server address: %s\n",ip_addr);

		//Set timeout
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		struct sockaddr_in serveraddr, tempaddr;
		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = inet_addr(ip_addr);
		
		unsigned int addrlength;
		int fileindex = -1;  //Last index read from file
		int lar = -1;
		int endfile = 0;
		int lfs;
		int aws = windowsize;
		
		while(1) {
			if (endfile == 1 && fileindex == lar)
				break;
			
			printf("Current aws: %d\n", aws);
			while (fileindex - lar < buffersize && endfile == 0) {
				unsigned char data;
				int status = fread(&data,sizeof(char),1,fi);
				if (status == 0) {
					endfile = 1;
				}
				else {
					fileindex++;
					buffer[fileindex % buffersize] = data;
				}
			}
			
			if (aws == 0) {
				printf("Sleep for 1 sec\n");
				usleep(1000);
				aws = 1; //dummy for checking reciver's aws
			}
			lfs = min(fileindex, lar+windowsize);
			int lastsending = min(lfs, lar + aws);
			for (int i = lar + 1; i <= lastsending; i++) {
				packet[0] = 0x1;
				memcpy(packet+1, &i, sizeof(int));
				packet[5] = 0x2;
				memcpy(packet+6, &buffer[i % buffersize], sizeof(char));
				packet[7] = 0x3;
				packet[8] = encryptCRC(packet, 8);
				
				if (sendto(fd, packet, 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
					perror("sendto failed");
					break;
				}
				printf("Data %d sent: %c\n", i, buffer[i % buffersize]);
			}
			for (int i = lar+1; i <= lastsending; i++) {
				if (recvfrom(fd, &ack, 7, 0, NULL, 0) >= 0) {
					if (decryptCRC(ack, 7) == 1) {
						int acknum;
						memcpy(&acknum, ack+1, sizeof(int));
						memcpy(&aws, ack+5, sizeof(char));
						printf("Received ACK %d, aws %d\n", acknum, aws);
						lar = acknum;
					}
					else {
						printf("Corrupted ACK recieved");
					}
				} else {
					printf("Timeout\n");
				}
			}
			printf("\n");
		}
		int eofindex = -1;
		unsigned char eofdata = 0;
		packet[0] = 0x1;
		memcpy(packet+1, &eofindex, sizeof(int));
		packet[5] = 0x2;
		memcpy(packet+6, &eofdata, sizeof(char));
		packet[7] = 0x3;
		packet[8] = encryptCRC(packet, 8);
		sendto(fd, packet, 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

		fclose(fi);
		close(fd);
	}
	else {
		printf("Enter arguments: <filename> <windowsize> <buffersize> <server_IP_address> <port_number>\n");
	}
	return 0;
}
