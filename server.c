#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE (10)
#define SERVERPORT (5000)
#define MAXPENDING (5)

void handleconnection(int clientsock);

int main() {
	int servsock;
	struct sockaddr_in servaddress;
	in_port_t port = SERVERPORT;
	
	//Create Socket
	if((servsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("socket() failed\n");
		exit(1);
	}

	//Construct address
	memset(&servaddress, 0, sizeof(servaddress));
	servaddress.sin_family = AF_INET;
	servaddress.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddress.sin_port = htons(port);

	//bind socket
	if (bind(servsock, (struct sockaddr *) &servaddress, sizeof(servaddress)) < 0){
		printf("bind() failed\n");
		exit(1);
	}

	//listen on socket
	if (listen(servsock, MAXPENDING) < 0){
		printf("listen() failed\n");
		exit(1);
	}

	//forever loop
	for(;;){
		int clientsock;
		pid_t result;
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);

		if ((clientsock = accept(servsock, (struct sockaddr *) &clientaddr, &clientaddrlen)) < 0){
			printf("accept() failed\n");
			exit(1);
		}

		result = fork();
		if (result == 0){
			close(servsock);
			handleconnection(clientsock);
		} else if (result > 0) {
			close(clientsock);
		} else if (result < 0){
			printf("fork() failed\n");
			exit(1);
		}
	}
}

void handleconnection(int clientsock){
	exit(0);
}
