#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include "../../include/common/common.h"
#include "../../include/linkedlist/linkedlist.h"
	

int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	socklen_t clilen;
	pthread_t thread_id;
	struct sockaddr_in serv_addr, cli_addr;
	
	createList(clientList);//Criando a lista

	printf("Opening Socket...\n");

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr,"ERROR opening socket.\n");
		exit(-1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	printf("Binding Socket...\n");

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr,"ERROR on binding.\n");
		exit(-1);
	}
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);

	printf("Accepting new connections...\n");

	while ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) != -1) {
		printf("Connection Accepted\n");

		if(pthread_create(&thread_id, NULL, handleConnection, (void*)&newsockfd) < 0){
			fprintf(stderr,"ERROR, could not create thread.\n");
			exit(-1);
		}

		printf("Handler Assigned\n");

	}
		
	close(sockfd);
	return 0; 
}
