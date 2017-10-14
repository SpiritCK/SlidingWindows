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
	if (argc == 2) {
		int fd, port;
		if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
			perror( "socket failed" );
			return 1;
		}
		port = strtol(argv[1], '\0', 10);
		printf("Port: %d\n",port);

		struct sockaddr_in serveraddr;
		memset( &serveraddr, 0, sizeof(serveraddr) );
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );

		if ( bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0 ) {
			perror( "bind failed" );
			return 1;
		}

		char buffer[200];
		while(1) {
			int length = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, NULL, 0);
			if ( length < 0 ) {
				perror( "recvfrom failed" );
				break;
			}
			buffer[length] = '\0';
			printf("Recieved %d bytes: '%s'\n", length, buffer);
			if (strcmp("end", buffer) == 0)
				break;
		}

		close( fd );
	}
	else {
		printf("Enter arguments: <port_number>\n");
	}
	return 0;
}
