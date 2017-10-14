 #include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	if (argc == 3) {
		int fd, port;
		char input[200];
		if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
			perror("Socket failed");
			return 1;
		}
		port = strtol(argv[2], '\0', 10);
		printf("Port: %d\n",port);
		printf("Server address: %s\n",argv[1]);

		struct sockaddr_in serveraddr;
		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

		while(1) {
			printf("Enter message: ");
			scanf("%s",input);
			int size = 0;
			while (input[size] != '\0') {
				size++;
			}
			if (sendto( fd, input, size, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
				perror( "sendto failed" );
				break;
			}
			printf( "Message sent\n" );
			if (strcmp("end", input) == 0)
				break;
		}

		close( fd );
	}
	else {
		printf("Enter arguments: <server_IP_address> <port_number>\n");
	}
	return 0;
}
