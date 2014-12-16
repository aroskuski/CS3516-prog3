//Unless otherwise noted all methods were written by Adam Ansel

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
unsigned short charstoshort(unsigned char char1, unsigned char char2);


char addr[INET_ADDRSTRLEN];
unsigned char seq_num[2] = {0,1};
FILE *logfile;
long long errorcounter = 0;
long long frame_count = 0;
long long resend_frame_count = 0;
long long packet_count = 0;


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

// application layer and newtork layer are in this function
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

  struct timeval start, stop;
  gettimeofday(&start, NULL);


  //
  int i;
  int j;
  for (j = 0; j < numofPhotos; j++)
  {
    photo_len = sprintf(photo_name, "%s%d%d.%s", PHOTO, client_id, j + 1, PHOTO_EXT);
    //printf("%s\n", photo_name);

    if((file = open(photo_name, O_RDONLY)) < 0){
      printf("File failed to open\n");
      exit(1);
    }

    int rd_size = 0;
    unsigned char rd_buffer[BUFSIZE];
    int pkt_size = 0;
    unsigned char pkt_buffer[BUFSIZE];
    rd_size = read(file, rd_buffer, BUFSIZE - 1);

    // Netwok layer packets are all 1-200 bytes of payload, with one End of Picture byte at the end
    while(rd_size > 0){
      for(i = 0; i < rd_size; i++){
        pkt_buffer[i] = rd_buffer[i];
      }
      pkt_size = rd_size + 1;
      rd_size = read(file, rd_buffer, BUFSIZE - 1);// The call to read is here so it's return value can be checked
      if(rd_size > 0){
        pkt_buffer[pkt_size - 1] = NOT_EOP;
      }
      else{
        pkt_buffer[pkt_size - 1] = EOP;
      }
      printtolog("Packet created, Sending to Datalink Layer\n");
      int ack_length;
      packet_count++;
      // send packet data link layer and receive ack
      ack_length = dll_send(sock, pkt_buffer, pkt_size);
      //Network layer acks are just one byte that contains FT_ACK
      if(pkt_buffer[0] != FT_ACK){//Something went wrong, exitting
        exit(1);
      }
    }
  }

  //fputc('\n', stdout); // Print a final linefeed
  printtolog("Client disconnected from server\n");

  char count[10000];
  printtolog("Number of frames sent: ");
  sprintf(count, "%lld", frame_count);
  printtolog(count);
  printtolog("\n");

  printtolog("Number of frames resent: ");
  sprintf(count, "%lld", resend_frame_count);
  printtolog(count);
  printtolog("\n");

  gettimeofday(&stop, NULL);
  struct timeval elapsed;
  timersub(&stop, &start, &elapsed);
  long long total_time = 0;
  total_time = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);
  char elapsed_time[100];
  printtolog("Process took ");
  sprintf(elapsed_time, "%lld", total_time);
  printtolog(elapsed_time);
  printtolog(" milliseconds\n");

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

/*
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
*/

//int nwl_recv(int sockfd, unsigned char* buffer, unsigned int buffer_len){
//
//}

//encapsulates the packet in frames, sends to physical layer
int dll_send(int sockfd, unsigned char* buffer, int buffer_len){
  int i;
  int buf_pos = 0;
  int frame_size;
  int nwl_ack_length = 0;
  int ack_length;
  unsigned char ack[MAX_FRAME_SIZE];
  unsigned char nwl_ack[MAX_FRAME_SIZE];
  unsigned char frame[MAX_FRAME_SIZE];
  unsigned char ed[2];

  //printf("Buffer Length=%d\n", buffer_len);
  while(buf_pos < buffer_len){

    frame[0] = seq_num[0];
    frame[1] = seq_num[1];
    frame[2] = FT_DATA;
    //printf("%d\n", buffer_len - buf_pos);
    if((buffer_len - buf_pos) <= 124){// If the remainder of th buffer can fit in a single frame
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

    char seq[10];
    printtolog("Sending frame ");
    sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
    printtolog(seq);
    printtolog(" \n");

    printtolog("Frame ");
    sprintf(seq, "%lld", frame_count + 1);
    printtolog(seq);
    printtolog(", part of Packet ");
    sprintf(seq, "%lld", packet_count);
    printtolog(seq);
    printtolog(", being sent\n");

    
    struct timeval timeout;
    fd_set bvfdRead;
    int readyNo;
    int waiting = 1;

    errorcounter++;
    if(errorcounter == 5){
      frame[frame_size - 1] ^= 1;
    }

    phl_send(sockfd, frame, frame_size);
    frame_count++;

    if(errorcounter == 5){
      frame[frame_size - 1] ^= 1;
      errorcounter = 0;
    }


    while(waiting){
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      FD_ZERO(&bvfdRead);
      FD_SET(sockfd, &bvfdRead);
      readyNo = select(sockfd + 1, &bvfdRead, NULL, NULL, &timeout);

      if(readyNo < 0){ //error
        printf("Select failed\n");
        exit(1);
      }
      else if(readyNo == 0){//timeout
        printtolog("Ack ");
        sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
        printtolog(seq);
        printtolog(" was not received and timeout occurred\n");

        printtolog("Resending frame ");
        sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
        printtolog(seq);
        printtolog(" \n");

        printtolog("Frame ");
        sprintf(seq, "%lld", frame_count);
        printtolog(seq);
        printtolog(", part of Packet ");
        sprintf(seq, "%lld", packet_count);
        printtolog(seq);
        printtolog(", being resent\n");

        phl_send(sockfd, frame, frame_size);
        resend_frame_count++;
      }
      else{//socket ready
        FD_ZERO(&bvfdRead);
        ack_length = phl_recv(sockfd, ack, MAX_FRAME_SIZE);

        if(ack[2] != FT_ACK){// this is actually network layer ack
          int j;
          for(j = 4; j < ack_length - 2; j++){
            nwl_ack[j - 4] = ack[j];
            nwl_ack_length++;
          }
        }
        else{
          if((ack[0] == ack[3]) && (ack[1] == ack[4])){//ack valid
            printtolog("Ack ");
            sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
            printtolog(seq);
            printtolog(" was received\n");

            incrementSQ();
            waiting = 0;
          }
          else{//ack invalid
            printtolog("Ack ");
            sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
            printtolog(seq);
            printtolog(" was not received\n");

            printtolog("Resending frame ");
            sprintf(seq, "%d", charstoshort(frame[0], frame[1]));
            printtolog(seq);
            printtolog(" \n");

            printtolog("Frame ");
            sprintf(seq, "%lld", frame_count);
            printtolog(seq);
            printtolog(", part of Packet ");
            sprintf(seq, "%lld", packet_count);
            printtolog(seq);
            printtolog(", being resent\n");            

            phl_send(sockfd, frame, frame_size);
            resend_frame_count;
          }
        }
      }
    }
  }

  if(nwl_ack_length == 0){// If network layer ack hasn't been received yet
    ack_length = phl_recv(sockfd, ack, MAX_FRAME_SIZE);
    int j;
    for(j = 4; j < ack_length - 2; j++){
      nwl_ack[j - 4] = ack[j];
      nwl_ack_length++;
    }
  }

  for(i = 0; i < nwl_ack_length; i++){
    buffer[i] = nwl_ack[i];
  }


  return nwl_ack_length;
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

//sends a frame over TCP
int phl_send(int sockfd, unsigned char* buffer, int buffer_len){
  printtolog("Sending data to server\n");
  return send(sockfd, buffer, buffer_len, 0);
}

//recieves a frame over TCP
int phl_recv(int sockfd, unsigned char* buffer, int buffer_len){
  return recv(sockfd, buffer, buffer_len, 0);
}

// increments the sequence number
void incrementSQ(){
  seq_num[1]++;
  if(seq_num[1] == 0){
    seq_num[0]++;
  }
}

//Method written by Andrew Roskuski
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

//Method written by Andrew Roskuski
void printtolog(char *logtext){
  fputs(logtext, logfile);
  fflush(logfile);
  //fputc('\n', logfile);
}

//Method written by Andrew Roskuski
unsigned short charstoshort(unsigned char char1, unsigned char char2){
  unsigned short result = (((unsigned short)char1) << 8) | char2;
  return result;
}

