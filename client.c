#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "BaseMessage.h"
#include "Player.h"

#define MAXLINE 1024
#define PORT 13370

int send_message(struct sockaddr_in servaddr, int sock_fd, BaseMessage* msg)
{
	sendto(sock_fd, (const char *)msg, sizeof(BaseMessage), 
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
			sizeof(servaddr)); 
}

int send_move(struct sockaddr_in servaddr, int sock_fd, int client_id, int x, int y)
{
	BaseMessage msg;
	msg.type = MSG_MOVE;
	msg.move.id = client_id;
	msg.move.x = x;
	msg.move.y = y;
	send_message(servaddr, sock_fd, &msg);

}

int send_join(struct sockaddr_in servaddr, int sock_fd)
{
	BaseMessage msg;
	msg.type = MSG_JOIN;
	send_message(servaddr, sock_fd, &msg);
}

int send_quit(struct sockaddr_in servaddr, int sock_fd, int id)
{
	BaseMessage msg;
	msg.type = MSG_QUIT;
	msg.quit.id = id;
	send_message(servaddr, sock_fd, &msg);
}

int send_close(struct sockaddr_in servaddr, int sock_fd)
{
	BaseMessage msg;
	msg.type = MSG_CLOSE;
	send_message(servaddr, sock_fd, &msg);
}

int expectACK(struct sockaddr* servaddr, int sockfd) 
{
	int n, len; 
	BaseMessage response;
	n = recvfrom(sockfd, (char *)&response, MAXLINE,  
			MSG_WAITALL, servaddr, 
			&len); 
	if (response.type == MSG_ACK) {
		printf("CLIENT: server expectedly acknowledged last message.\n");
		return 0;
	} else {
		printf("CLIENT: server unexpectedly responded with message of type %d\n", response.type);
		return -1;
	}
}

int expectACKJoin(struct sockaddr* servaddr, int sockfd, int* client_id)
{
	int n, len; 
	BaseMessage response;
	n = recvfrom(sockfd, (char *)&response, MAXLINE,  
			MSG_WAITALL, servaddr, 
			&len); 
	if (response.type == MSG_ACKJOIN) {
		printf("CLIENT: server assigned us id %d.\n", response.ackjoin.id);
		*client_id = response.ackjoin.id;
		return 0;
	} else {
		printf("CLIENT: server unexpectedly responded with message of type %d\n", response.type);
		return -1;
	}
}

int expectSet(struct sockaddr* servaddr, int sockfd, Player* players)
{
	int n, len; 
	BaseMessage response;
	n = recvfrom(sockfd, (char *)&response, MAXLINE,  
			MSG_WAITALL, servaddr, 
			&len); 
	if (response.type == MSG_SET) {
		for (int i = 0; i < 10; ++i) {
			players[i].px = response.set.entries[i].x;
			players[i].py = response.set.entries[i].y;
		}
		return 0;
	} else {
		printf("CLIENT: server unexpectedly responded with message of type %d\n", response.type);
		return -1;
	}
}

int start_SDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("SDL_Init error: '%s'\n", SDL_GetError());
		return -1;
	}
	return 0;
}

void draw_player(SDL_Renderer* renderer, Player* player, bool highlight)
{
	SDL_Rect rect = {player->px - 5, player->py - 5, 10, 10 };
	if (highlight) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	} else {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	}
		
	SDL_RenderDrawRect(renderer, &rect);
}

int main(int argc, char** argv)
{
	int sockfd; 
	char buffer[MAXLINE]; 
	char *hello = "Hello from client"; 
	struct sockaddr_in     servaddr; 
	int client_id = 0;

	Player players[10];
	for (int i = 0; i < 10; ++i) {
		players[i].id = i;
		players[i].px = i * 40;
		players[i].py = i * 40;
	}

	if (start_SDL() < 0) {
		return -1;
	}
	SDL_Window* window = SDL_CreateWindow("Just Dots!", 200, 200, 640, 480, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer error: '%s'\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&servaddr, 0, sizeof(servaddr)); 

	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(PORT); 
	servaddr.sin_addr.s_addr = INADDR_ANY; 

	send_join(servaddr, sockfd);
	expectACKJoin((struct sockaddr*) &servaddr, sockfd, &client_id);

	bool will_close = false;
	SDL_Event e;
	while (! will_close) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT:
					will_close = true;
					break;
				case SDL_MOUSEMOTION:
					players[client_id].px = e.motion.x;
					players[client_id].py = e.motion.y;
					break;
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		for (int i = 0; i < 10; ++i) {
			draw_player(renderer, &(players[i]), i == client_id);
		}
		SDL_RenderPresent(renderer);
		send_move(servaddr, sockfd, client_id, players[client_id].px, players[client_id].py);
		expectSet((struct sockaddr*) &servaddr, sockfd, players);
		SDL_Delay(50);
	}
	send_quit(servaddr, sockfd, client_id);
	expectACK((struct sockaddr*) &servaddr, sockfd);
	printf("Move message sent.\n"); 

	close(sockfd); 
	SDL_Quit();
	return 0;
}

