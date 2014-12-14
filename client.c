#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "photo.h"


#define BUFSIZE 256
#define ACK "Packet"
#define PHOTO "photo"
#define PHOTO_EXT "jpg"
#define STOP "stop"
#define NEXT "next file"

#define PACKET_SIZE 200
#define MAX_FRAME_SIZE 130

char *getIPbyHostName(char *servName, char *addr);
int confirm_ack(char* ack);
char addr[INET_ADDRSTRLEN];

int connect_to(char *serverName, unsigned short severPort){

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                          // Server address
  char *servName = serverName;
  char *servIP = getIPbyHostName(servName, addr);         //Get Server IP by Server Name
  struct sockaddr_in servAddr; 

  // Create socket stream
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;     

  // Convert address
  int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (rtnVal == 0){
    printf("inet_pton() failed: invalid address string\n");
  }
  else if (rtnVal < 0){
    printf("inet_pton() failed\n");
  }
  servAddr.sin_port = htons(SERVERPORT);


  // establish connection
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
    printf("connect() failed\n");
  }
  return sock;
}


int main(int argc, char *argv[]) {

  int sock;
  int client_id = 0;
  int numofPhotos = 0;
  int file = 0;

  char *photo_name;
  unsigned int photo_len;
  char ack[BUFSIZE];
  int bytes_rcvd;
  int total_bytes_rcvd;
  

  if (argc < 4) {
    printf("Parameter(s): <Server Name> <Client ID> <Number of Photos>\n");
    exit(1);
  }

  if ((sock = connect_to(argv[1], SERVERPORT)) < 0) {
    printf("socket() failed\n");
    exit(1);
  }


  // photo handling
  client_id = atoi(argv[2]);
  numofPhotos = atoi(argv[3]);
  int i;

  for (i = 0; i < numofPhotos; i++)
  {
    photo_len = sprintf(photo_name, "%s%d%d.%s", PHOTO, client_id, i, PHOTO_EXT);
    printf("%s\n", photo_name);

    if((file = open(photo_name, O_RDONLY)) < 0){
      printf("File open\n");
      exit(1);
    }

    int rd_size = 0;
    char rd_buffer[BUFSIZE];

    if(send(sock, photo_name, photo_len, 0) != photo_len){
      printf("send(): sent unexpected number of bytes\n");
      exit(0);
    }
    if(bytes_rcvd = recv(sock, ack, BUFSIZE - 1, 0) <= 0){
      printf("recv() failed\n");
    }

    confirm_ack(ack);
    while((rd_size = read(file, rd_buffer, BUFSIZE)) > 0){
      if(send(sock, rd_buffer, rd_size, 0) != rd_size){
        printf("send(): sent unexpected number of bytes\n");
        exit(0);
      }
      if((bytes_rcvd = recv(sock, ack, BUFSIZE - 1, 0)) <= 0){
        printf("recv() failed\n");
      }

      ack[bytes_rcvd] = '\0';
      confirm_ack(ack);
    }

    if(i == numofPhotos - 1){
      if(send(sock, STOP, strlen(STOP), 0) != strlen(STOP)){
        printf("send(): sent unexpected number of bytes\n");
        exit(0);
      }
    }
    else{
      if(send(sock, NEXT, strlen(NEXT), 0) != strlen(NEXT)){
        printf("send(): sent unexpected number of bytes\n");
        exit(0);
      }
    }
    if((bytes_rcvd = recv(sock, ack, BUFSIZE - 1, 0)) <= 0){
      printf("recv() failed\n");
      exit(0);
    }
    confirm_ack(ack);
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

int confirm_ack(char* ack){
  if(strcmp(ack, ACK) != 0){
    printf("Ack not received\n");
    exit(0);
    return 0;
  }
  return(1);
}


int nwl_send(int sockfd, unsigned char* buffer, unsigned int buffer_len){
  unsigned char packet[PACKET_SIZE + EOP];
  int i;
  int j = 0;
  int size;
  for(i = 0; i < buffer_len; i++){
    packet[j] = buffer[i];
    if((j == PACKET_SIZE - 1) && (i != buffer_len - 1)){
      packet[j + 1] = NOT_EOP;
      dll_send(sockfd, packet, j + 2);
      size = dll_recv(sockfd, packet, PACKET_SIZE + EOP);
      if(packet[0] != FT_ACK){
        printf("Error: Packet is not an ack\n");
        exit(1);
      }
      j = 0;
    }
    else if(i == buffer_len - 1){
      packet[j + 1] = EOP;
      dll_send(sockfd, packet, j + 2);
      size = dll_recv(sockfd, packet, PACKET_SIZE + EOP);
      if(packet[0] != FT_ACK){
        printf("Error: Packet is not an ack\n");
        exit(1);
      }
      j = 0;
    }
    else{
      j++;
    }
  }

  return 0;
}

//int nwl_recv(int sockfd, unsigned char* buffer, unsigned int buffer_len){
//
//}


int dll_send(int sockfd, unsigned char* buffer, int buffer_len){
  int i;
  int j;
  unsigned char frame[MAX_FRAME_SIZE];




}

int dll_recv(int sockfd, unsigned char* buffer, int buffer_len){

}

int phl_send(int sockfd, unsigned char* buffer, int buffer_len){
  return send(sockfd, buffer, buffer_len, 0);
}

int phl_recv(int sockfd, unsigned char* buffer, int buffer_len){
  return recv(sockfd, buffer, buffer_len, 0);
}

