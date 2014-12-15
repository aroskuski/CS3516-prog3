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


#define BUFSIZE 201
#define ACK "Packet"
#define PHOTO "photo"
#define PHOTO_EXT "jpg"

#define PACKET_SIZE 200
#define MAX_FRAME_SIZE 130

char *getIPbyHostName(char *servName, char *addr);
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

  char photo_name[5000];
  unsigned int photo_len;
  char buffer[BUFSIZE];
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
      printf("File failed to open\n");
      exit(1);
    }

    int rd_size = 0;
    unsigned char rd_buffer[BUFSIZE];
    int pkt_size = 0;
    unsigned char pkt_buffer[BUFSIZE];
    rd_size = read(file, rd_buffer, BUFSIZE - 1);

    while(rd_size > 0){
      for(i = 0; i < rd_size; i++){
        pkt_buffer[i] = rd_buffer[i];
      }
      pkt_size = rd_size + 1;
      rd_size = read(file, rd_buffer, BUFSIZE - 1);
      if(rd_size > 0){
        pkt_buffer[pkt_size - 1] = NOT_EOP;
      }
      else{
        pkt_buffer[pkt_size - 1] = EOP;
      }
      dll_send(sock, pkt_buffer, pkt_size);
    }
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
    exit(1);
  }

  ph = host -> h_addr_list;
  inet_ntop(AF_INET, *ph, addr, INET_ADDRSTRLEN);
  return addr;

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
  int buf_pos;
  int frame_size;
  unsigned char ack;
  unsigned char seq_num[2];
  unsigned char frame[MAX_FRAME_SIZE];
  unsigned char ed[2];

  while(buf_pos < buffer_len){
    frame[0] = 0;
    frame[1] = 1;
    frame[2] = FT_DATA;
    printf("%d\n", buffer_len - buf_pos);
    if((buffer_len - buf_pos) <= 124){
      frame[3] = EOP;
    }
    else{
      frame[3] = NOT_EOP;
    }
    frame_size = 4;

    for(i = 4; i < 128 && buf_pos < buffer_len; i++){
      frame[i] = buffer[buf_pos];
      buf_pos++;
      frame_size++;
    }
    frame_size += 2;
    frame[frame_size - 2] = 0;
    frame[frame_size - 1] = 1;

    phl_send(sockfd, frame, frame_size);

    frame_size = phl_recv(sockfd, frame, MAX_FRAME_SIZE);
  }
}

int dll_recv(int sockfd, unsigned char* buffer, int buffer_len){

}

int phl_send(int sockfd, unsigned char* buffer, int buffer_len){
  return send(sockfd, buffer, buffer_len, 0);
}

int phl_recv(int sockfd, unsigned char* buffer, int buffer_len){
  return recv(sockfd, buffer, buffer_len, 0);
}

