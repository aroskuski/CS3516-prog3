#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>


#define BUFSIZE 10
#define SERVERPORT (5000)

char addr[INET_ADDRSTRLEN];

int main(int argc, char *argv[]) {

  char *servName = argv[1];
  char *servIP = getIPbyHostName(servName, addr);   //Get Server IP by Server Name

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock < 0)
    DieWithError("socket() failed");  

  // Create socket stream
  struct sockaddr_in servAddr;            // Server address
  memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
  servAddr.sin_family = AF_INET;          // IPv4 address family

  // Convert address
  int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (rtnVal == 0)
    printf("inet_pton() failed: invalid address string\n");
  else if (rtnVal < 0)
    printf("inet_pton() failed\n");
  servAddr.sin_port = htons(servPort);    // Server port


  // establish connection
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    printf("connect() failed\n");

  //
  unsigned int totalBytesRcvd = 0;
  char buffer[BUFSIZE];
  ssize_t numBytes = send(sock, buffer, BUFSIZE, 0);;
  
  while(totalBytesRcvd < BUFSIZE){
    numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
  }

  fputc('\n', stdout); // Print a final linefeed

  close(sock);
  exit(0);
}