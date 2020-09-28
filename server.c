#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdbool.h>
#include "BaseMessage.h"
#include "Player.h"

#define MAXLINE 1024
#define PORT 13370

int main(int argc, char** argv)
{
	int current_client_id = 0;
	int sockfd;
	char buffer[MAXLINE];
	char *hello = "Hello from server";
	struct sockaddr_in servaddr, cliaddr;

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family    = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
				sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	int len, n;
	bool running = true;

	Player players[10];
	for (int i = 0; i < 10; ++i) {
		players[i].px = i * 20;
		players[i].py = i * 20;
	}

	len = sizeof(cliaddr);  //len is value/resuslt

	while (running) {
		BaseMessage msg;
		BaseMessage resp;
		n = recvfrom(sockfd, (char *)&msg, MAXLINE,
				MSG_WAITALL, ( struct sockaddr *) &cliaddr,
				&len);
		switch (msg.type) {
			case MSG_JOIN:
				resp.type = MSG_ACKJOIN;
				resp.ackjoin.id = current_client_id++;
				printf("SERVER: Client joined. Assigned id %d\n", resp.ackjoin.id);
				break;
			case MSG_MOVE:
				resp.type = MSG_SET;
				if (msg.move.id >= 0 && msg.move.id < 10) {
					players[msg.move.id].px = msg.move.x;
					players[msg.move.id].py = msg.move.y;
				}
				for (int i = 0; i < 10; ++i) {
					resp.set.entries[i].x = players[i].px;
					resp.set.entries[i].y = players[i].py;
					resp.set.entries[i].exists = true;
				}
				break;
			case MSG_QUIT:
				resp.type = MSG_ACK;
				printf("SERVER: Client %d quits\n", resp.quit.id);
				break;
			case MSG_CLOSE:
				running = false;
				resp.type = MSG_ACK;
				printf("SERVER: received CLOSE, closing down...\n");
				break;
		}
		sendto(sockfd, (const char *)&resp, sizeof(BaseMessage),
				MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
				len);
	}

	return 0;
}

