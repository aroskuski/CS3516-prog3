#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include "photo.h"

#define BUFSIZE (130)

#define MAXPENDING (5)

struct frameseq {
	unsigned char seq1;
	unsigned char seq2;
	struct frameseq* next;
};

void handleconnection();
int phl_recv();
void dll_recv(unsigned char *frame, int size);
void nwl_recv(unsigned char *packet, int size);
void app_recv(unsigned char *photo, int size);
//void nwl_send(unsigned char *photo, int size);
void dll_send(unsigned char *packet, int size);
void phl_send(unsigned char *frame, int size);
int totalwindowsize();
void printtolog(char *logtext);
int errorcheck(unsigned char *frame, int size);
void generateED(unsigned char *frame, int size, unsigned char *ed);
int checkdup(unsigned char seq1, unsigned char seq2);
void keeplistsmall();
void addframeseq(unsigned char seq1, unsigned char seq2);
void freelist(struct frameseq *seq);

FILE *logfile;
FILE *outfile;
int currfile = 0;
int outclosed = 1;
int clientsock;
unsigned char *framewindow[10];
unsigned char framewindowseq[10][2];
int framewindowsize[10];
int framewindownext = 0;
int clientid = 0;
struct frameseq *frameseqhead = NULL;

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
		//int clientsock;
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
			clientid++;
		} else if (result < 0){
			printf("fork() failed\n");
			exit(1);
		}
	}
}

void handleconnection(int clientsock){
	//if(fcntl(clientsock, F_SETFL, fcntl(clientsock, F_GETFL) | O_NONBLOCK) < 0) {
		// handle error
	//}
	char logfilename[20];
	//char outfilename[20];
	char id[5];
	strcpy(logfilename, "server_");
	sprintf(id, "%d", clientid);
	strcat(logfilename, id);
	strcat(logfilename, ".log");
	logfile = fopen(logfilename, "w");

	
	
	if (logfile = NULL){
		printf("failed to open logfile\n");
		exit(1);
	}

	int connection_open = 1;
	while (connection_open){
		connection_open = phl_recv(clientsock);
		keeplistsmall();
	}
	
	freelist(frameseqhead);
	fclose(logfile);
	exit(0);
}

int phl_recv(int clientsock) {
	//fd_set socketset;
	int bytes_recved = 0;
	//unsigned char buf[BUFSIZE];
	unsigned char frame[BUFSIZE];
	int i;
	ssize_t result;
	//int nodata_count = 0;
	int frame_recved = 0;
	//struct timeval timeout;
	//struct timeval timeout_tmp;
	//timeout.tv_sec = 1;
	//timeout.tv_usec = 0;
	

	result = recv(clientsock, frame, BUFSIZE, 0);
	if (result == 0){
		return 0;
	} else if (result < 0){
		printf("recv() failed\n");
		exit(1);
	} else {
		//nodata_count = 0;
		//for(i = bytes_recved; i < result + bytes_recved; i++){
		//	frame[i] = buf[i - bytes_recved];
		//}
		bytes_recved = result;
	}

	
	dll_recv(frame, bytes_recved);
	return 1;
}

void dll_recv(unsigned char *frame, int size){
	int eop = 0;
	int i;
	int j;
	int k = 0;
	int dup;

	//if(errorcheck(frame, size)){
	//	return;
	//}

	//for(i = 0; i < framewindownext; i++){
	//	if((frame[0] == framewindowseq[i][0])  && (frame[1] == framewindowseq[i][1])){
	//		dup = 1;
	//	}
	//}
	dup = checkdup(frame[0], frame[1]);
		
	unsigned char ack[5];
	ack[0] = frame[0];
	ack[1] = frame[1];
	ack[2] = FT_ACK;
	ack[3] = frame[0];
	ack[4] = frame[1];
	phl_send(ack, 5);

	if(dup){
		return;
	}

	if(frame[3] == EOP){
		eop = 1;
	}

	framewindow[framewindownext] = malloc(size - 6);
	framewindowsize[framewindownext] = size - 6;
	//framewindowseq[framewindownext][0] = frame[0];
	//framewindowseq[framewindownext][1] = frame[1];
	addframeseq(frame[0], frame[1]);	
	for(i = 4; i < size - 2; i++){
		framewindow[framewindownext][i] = frame[i];
	}
	framewindownext++;

	if (eop){
		//reassemble packet
		unsigned char *packet = malloc(totalwindowsize());
		for (i = 0; i < framewindownext; i++){
			for (j = 0; j < framewindowsize[i]; j++){
				packet[k] = framewindow[i][j];
				k++;
			}
		}

		nwl_recv(packet, k);

		//clear frame buffers
		free(packet);
		for (i = 0; i < framewindownext; i++){
			free(framewindow[i]);
		}
		framewindownext = 0;
	}
}

void nwl_recv(unsigned char *packet, int size){
	int eop = 0;
	//unsigned char *packet = dll_recv();

	if (outclosed){
		char outfilename[20];
		char id[5];
		strcpy(outfilename, "photonew");
		sprintf(id, "%d", clientid);
		strcat(outfilename, id);
		sprintf(id, "%d", currfile);
		strcat(outfilename, id);
		strcat(outfilename, ".jpg");
		outfile = fopen(outfilename, "wb");
		outclosed = 0;
	}
	

	unsigned char ack[1];
	ack[0] = FT_ACK;
	//ack[1] = packet[1];
	//ack[2] = FT_ACK;
	//ack[3] = packet[0];
	//ack[4] = packet[1];
	dll_send(ack, 1);

	if(packet[size - 1] == EOP){
		eop = 1;
	}

	//FILE *outfile = fopen("photonew.jpg", "a");
	if (fwrite(packet, 1, size - 1, outfile) == 0){
		printf("fwrite failed\n");
		exit(1);
	}

	if (eop){
		fclose(outfile);
		outclosed = 1;
		currfile++;
	}
}

void app_recv(unsigned char *photo, int size){
	FILE *outfile = fopen("photonew.jpg", "w+");
	if (fwrite(photo, 1, size, outfile) == 0){
		printf("fwrite failed\n");
		exit(1);
	}
}

/*
void nwl_send(unsigned char *photo){

}
*/

void dll_send(unsigned char *packet, int size){
	int i;
	unsigned char frame[130];
	unsigned char ed[2];
	frame[0] = 0;
	frame[1] = 1;
	frame[2] = FT_DATA;
	frame[3] = EOP;
	for (i = 0; i < size; i++){
		frame[i + 4] = packet[i];
	}
	generateED(frame, size + 6, ed);
	frame[(size + 6) - 2] = ed[0];
	frame[(size + 6) - 1] = ed[1];

	phl_send(frame, size + 6);
	
}

void phl_send(unsigned char *frame, int size){
	ssize_t sent = send(clientsock, frame, size, 0);
	if (sent != size){
		printf("send() failed\n");
		exit(1);
	}
}

int totalwindowsize(){
	int result;
	int i;
	for (i = 0; i < framewindownext; i++){
		result += framewindowsize[i];
	}
	return result;
}

void printtolog(char *logtext){
	fputs(logtext, logfile);
	fputc('\n', logfile);
}

int errorcheck(unsigned char *frame, int size){
	unsigned char checkbytes[2];
	int i;
	checkbytes[0] = 0;
	checkbytes[1] = 0;
	for(i = 0; i < (size - 2); i += 2){
		checkbytes[0] ^= frame[i];
		if (i < size -2){
			checkbytes[1] ^= frame[i + 1];
		}
	}
	return ((checkbytes[0] == frame[size - 2]) && (checkbytes[1] == frame[size - 1]));
}

void generateED(unsigned char *frame, int size, unsigned char *ed){
	int i;
	ed[0] = 0;
	ed[1] = 0;
	for(i = 0; i < (size - 2); i += 2){
		ed[0] ^= frame[i];
		if (i < size -2){
			ed[1] ^= frame[i + 1];
		}
	}
}

int checkdup(unsigned char seq1, unsigned char seq2){
	int result = 0;
	struct frameseq* ptr = frameseqhead;
	while(ptr != NULL){
		if (ptr->seq1 == seq1 && ptr->seq2 == seq2){
			result = 1;
		}
		ptr = ptr->next;
	}
	return result;
}

void keeplistsmall(){
	int i = 0;	
	struct frameseq* ptr = frameseqhead;
	while(ptr != NULL){
		i++;
		ptr = ptr->next;
	}

	if(i > 100){
		struct frameseq *temp = frameseqhead;
		frameseqhead = frameseqhead->next;
		free(temp);
	}
}

void addframeseq(unsigned char seq1, unsigned char seq2){
	int done = 0;	
	if (frameseqhead == NULL){
		frameseqhead = malloc(sizeof(struct frameseq));
		frameseqhead->seq1 = seq1;
		frameseqhead->seq2 = seq2;
		frameseqhead->next = NULL;
	} else {
		struct frameseq *ptr = frameseqhead;
		while(!done){
			if (ptr->next == NULL){
				ptr->next = malloc(sizeof(struct frameseq));
				ptr->next->seq1 = seq1;
				ptr->next->seq2 = seq2;
				ptr->next->next = NULL;
			} else {
				ptr = ptr->next;
			}
		}
	}
}

void freelist(struct frameseq *seq){
	if(seq != NULL){
		freelist(seq->next);
		free(seq);
	}
}
