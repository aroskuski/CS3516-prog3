#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include "photo.h"


#define BUFSIZE 10

char *getIPbyHostName(char *servName, char *addr);
char addr[INET_ADDRSTRLEN];

int main(int argc, char *argv[]) {

  char *servName = argv[1];
  char *servIP = getIPbyHostName(servName, addr);         //Get Server IP by Server Name
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in servAddr;                            // Server address
  

  if (argc < 2) {
    printf("Parameter(s): <Server Name> <Client ID> <Number of Photos>\n");
  }

  if (sock < 0) {
    printf("socket() failed\n");  
  }

  // Create socket stream
  memset(&servAddr, 0, sizeof(servAddr));     // Zero out structure
  servAddr.sin_family = AF_INET;              // IPv4 address family

  // Convert address
  int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (rtnVal == 0){
    printf("inet_pton() failed: invalid address string\n");
  }
  else if (rtnVal < 0){
    printf("inet_pton() failed\n");
  }
  servAddr.sin_port = htons(SERVERPORT);    // Server port


  // establish connection
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
    printf("connect() failed\n");
  }


  fputc('\n', stdout); // Print a final linefeed

  close(sock);
  exit(0);
}


char *getIPbyHostName(char *servName, char *addr)  
{
  struct hostent *host;
  char **ph;

  if ((host = gethostbyname(servName)) == NULL){
    printf("gethostbyname(): host connection failed\n");
    exit(0);
  }

  ph = host -> h_addr_list;
  inet_ntop(AF_INET, *ph, addr, INET_ADDRSTRLEN);
  return addr;

}
