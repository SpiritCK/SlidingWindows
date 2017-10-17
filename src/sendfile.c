 #include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "crc.h"

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

FILE* fl;
char* ip_addr;
//int makeError = 0;
//int errorEvery = 100;

void Log(char* tag, char* msg) {
	time_t ltime = time(NULL);
	struct tm result;
	char stime[32];
	localtime_r(&ltime, &result);
	asctime_r(&result, stime);
	stime[24] = 0;
	if (tag == NULL) {
		fprintf(fl, "%s -- [%s] [note] %s\n", ip_addr, stime, msg);
	}
	else {
		fprintf(fl, "%s -- [%s] [%s] %s\n", ip_addr, stime, tag, msg);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 6) {
		char logmsg[500];
		int sleepdelay = 100; //(ms)
		fl = fopen("log/sendlog.txt", "wb");
	
		char* filename = argv[1];
		int windowsize = strtol(argv[2], '\0', 10);
		int buffersize = strtol(argv[3], '\0', 10);
		ip_addr = argv[4];
		int port = strtol(argv[5], '\0', 10);
		
		sprintf(logmsg, "Sending %s with windowsize %d and buffersize %d", filename, windowsize, buffersize);
		Log("init", logmsg);
		
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
		sprintf(logmsg, "Set reciever %s at port %d", ip_addr, port);
		Log("init", logmsg);

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
					Log("buffer", "End of file reached");
				}
				else {
					fileindex++;
					buffer[fileindex % buffersize] = data;
					sprintf(logmsg, "Writing to buffer byte %d (index %d): 0x%02x", fileindex, fileindex % buffersize, data);
					Log("buffer", logmsg);
				}
			}
			
			if (aws == 0) {
				printf("Sleep for %d ms\n", sleepdelay);
				usleep(sleepdelay * 1000);
				aws = 1; //dummy for checking reciver's aws
			}
			lfs = min(fileindex, lar+windowsize);
			int lastsending = min(lfs, lar + aws);
			for (int i = lar + 1; i <= lastsending; i++) {
				unsigned char data = (buffer[i % buffersize])&0xff;
				sprintf(logmsg, "Reading from buffer byte %d (index %d): 0x%02x", i, i % buffersize, data);
				Log("buffer", logmsg);
				packet[0] = 0x1;
				memcpy(packet+1, &i, sizeof(int));
				packet[5] = 0x2;
				memcpy(packet+6, &data, sizeof(char));
				packet[7] = 0x3;
				/*if (makeError == errorEvery) {
					packet[8] = 0;
					makeError = 0;
				}
				else {*/
					packet[8] = encryptCRC(packet, 8);
					//makeError++;
				//}
				
				sprintf(logmsg, "Sending packet %d: 0x%02x", i, data);
				Log("packet", logmsg);
				if (sendto(fd, packet, 9, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
					perror("sendto failed");
					Log("ERROR", "sendto failed");
					break;
				}
				printf("Data %d sent: 0x%02x\n", i, data);
			}
			for (int i = lar+1; i <= lastsending; i++) {
				if (recvfrom(fd, &ack, 7, 0, NULL, 0) >= 0) {
					if (decryptCRC(ack, 7) == 1) {
						int acknum;
						memcpy(&acknum, ack+1, sizeof(int));
						memcpy(&aws, ack+5, sizeof(char));
						sprintf(logmsg, "ACK %d recieved with AWS: %d", acknum, aws);
						Log("ACK", logmsg);
						printf("Received ACK %d, aws %d\n", acknum, aws);
						if (acknum > lar) {
							sprintf(logmsg, "Moving window from %d to %d", lar, acknum);
							Log("window", logmsg);
							lar = acknum;
						}
					}
					else {
						Log("WARN", "Recieved corrupted ACK");
						printf("Corrupted ACK recieved");
					}
				} else {
					Log("WARN", "Timeout reached");
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
		Log("packet", "Sending EOF packet");

		fclose(fl);
		fclose(fi);
		close(fd);
	}
	else {
		printf("Enter arguments: <filename> <windowsize> <buffersize> <server_IP_address> <port_number>\n");
	}
	return 0;
}
