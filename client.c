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
#include <sys/file.h>
#include <sys/select.h>
#include <sys/time.h>
#include "photo.h"


#define BUFSIZE 201
#define ACK "Packet"
#define PHOTO "photo"
#define PHOTO_EXT "jpg"

#define PACKET_SIZE 200
#define MAX_FRAME_SIZE 130

int connect_to(char *serverName, unsigned short severPort);
char *getIPbyHostName(char *servName, char *addr);
int dll_send(int sockfd, unsigned char* buffer, int buffer_len);
int dll_recv(int sockfd, unsigned char* buffer, int buffer_len);
int phl_send(int sockfd, unsigned char* buffer, int buffer_len);
int phl_recv(int sockfd, unsigned char* buffer, int buffer_len);
void incrementSQ();
void generateED(unsigned char *frame, int size, unsigned char *ed);
void printtolog(char *logtext);


char addr[INET_ADDRSTRLEN];
unsigned char seq_num[2] = {0,0};
FILE *logfile;


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

  char logfilename[20];
  char id[5];
  strcpy(logfilename, "client_");
  sprintf(id, "%d", client_id);
  strcat(logfilename, id);
  strcat(logfilename, ".log");
  logfile = fopen(logfilename, "w");

  if(logfile == NULL){
    printf("failed to open logfile\n");
    exit(1);
  }

  char photo_count[100];
  printtolog("Connected to server");
  printtolog("\n");
  printtolog("Photos being sent: ");
  sprintf(photo_count, "%d", numofPhotos);
  printtolog(photo_count);
  printtolog("\n");




  int i;
  int j;
  for (j = 0; j < numofPhotos; j++)
  {
    photo_len = sprintf(photo_name, "%s%d%d.%s", PHOTO, client_id, j + 1, PHOTO_EXT);
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
      printtolog("Packet created, Sending to Datalink Layer\n");
      dll_send(sock, pkt_buffer, pkt_size);
      //dll_recv(ack);
    }
  }

  fputc('\n', stdout); // Print a final linefeed
  printtolog("Client disconnected from server");
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
  int buf_pos = 0;
  int frame_size;
  unsigned char ack[5];
  unsigned char frame[MAX_FRAME_SIZE];
  unsigned char ed[2];

  //printf("Buffer Length=%d\n", buffer_len);
  while(buf_pos < buffer_len){

    frame[0] = seq_num[0];
    frame[1] = seq_num[1];
    frame[2] = FT_DATA;
    //printf("%d\n", buffer_len - buf_pos);
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
    generateED(frame, frame_size, ed);
    frame[frame_size - 2] = ed[0];
    frame[frame_size - 1] = ed[1];

    printtolog("Frame created, Sending to Physical Layer\n");
    
    struct timeval timeout;
    fd_set bvfdRead;
    int readyNo;
    int waiting = 1;

    phl_send(sockfd, frame, frame_size);

    while(waiting){
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      FD_ZERO(&bvfdRead);
      FD_SET(sockfd, &bvfdRead);
      readyNo = select(sockfd + 1, &bvfdRead, NULL, NULL, &timeout);

      if(readyNo < 0){ 
        printf("Select failed\n");
        exit(1);
      }
      else if(readyNo == 0){
        phl_send(sockfd, frame, frame_size);
      }
      else{
        FD_ZERO(&bvfdRead);
        phl_recv(sockfd, ack, 5);

        if(ack[2] != FT_ACK){
          //
        }
        else{
          if((ack[0] == ack[3]) && (ack[1] == ack[4])){
            incrementSQ();
            waiting = 0;
          }
          else{
            phl_send(sockfd, frame, frame_size);
          }
        }
      }
    }

  }
}

int dll_recv(int sockfd, unsigned char* buffer, int buffer_len){
  /*
  printtolog("Received Ack");
  sprintf(seq_num, "%d", frame[0]);
  printtolog(seq_num);
  printtolog(", ");
  sprintf(seq_num, "%d", frame[1]);
  printtolog(seq_num);
  printtolog("\n");
  */

}

int phl_send(int sockfd, unsigned char* buffer, int buffer_len){
  printtolog("Sending data to server\n");
  return send(sockfd, buffer, buffer_len, 0);
}

int phl_recv(int sockfd, unsigned char* buffer, int buffer_len){
  return recv(sockfd, buffer, buffer_len, 0);
}

void incrementSQ(){
  seq_num[1]++;
  if(seq_num[1] == 0){
    seq_num[0]++;
  }
}

void generateED(unsigned char *frame, int size, unsigned char *ed){
  int i;
  ed[0] = 0;
  ed[1] = 0;
  for(i = 0; i < (size - 2); i += 2){
    ed[0] ^= frame[i];
    if (i + 1 < size -2){
      ed[1] ^= frame[i + 1];
    }
  }
}

void printtolog(char *logtext){
  fputs(logtext, logfile);
  fflush(logfile);
  //fputc('\n', logfile);
}

