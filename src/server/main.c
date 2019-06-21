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
#include "../../include/server/secondary.h"

#define DEBUG
#ifdef DEBUG
#define DEBUGPORT 4001
#endif
char ip[MAXNAME];	
int myPORT;

/*
PARA FAZER SETUP DO SERVIDOR MUDAR DEFINE DO WANTED_IP E PARA DEBUGAR DEBUGPORT
 */


int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	
	socklen_t clilen;
	pthread_t thread_id, thread_replica;
	struct sockaddr_in serv_addr, cli_addr;
	
	struct serverList* primaryServerNode;

	
	//Criando a listas
	createList(clientList);
	createServerList(serverList);

	// LENDO ARQUIVO .TXT E CRIANDO LISTA
	FILE *fp;
	long lSize;
	char *buffer;
	char *token;
	char ipToken[MAXNAME];
	char *portToken;

	// Pega ip próprio
	myIp(WANTED_IP, ip);

	fp=fopen("server_list.txt","r");
	if(fp==NULL){
		printf("\nFile Error\n");
		exit(-1);
	}
	// obtem tamanho do arquivo
  	fseek(fp,0,SEEK_END);
 	lSize=ftell(fp);
  	rewind(fp);
	// copia arquivo no buffer
  	buffer=(char*)malloc(sizeof(char)*lSize);	
  	fread(buffer,1,lSize,fp);

	if(strcspn(buffer, "\n")>0)
        buffer[strcspn(buffer, "\n")] = 0;
	// le tokens e adiciona a lista
	token = strtok(buffer,"-");
	while(token != NULL && strcmp(token,"")!=0){
		portToken = strrchr(token,';');
		portToken++;
		copyIp(token,ipToken);

		if(strcmp(ip,ipToken) == 0) {
			#ifndef DEBUG
				myPORT = atoi(portToken);
			#else
				myPORT = DEBUGPORT;
			#endif
		}

		insertServerList(&serverList,ipToken,atoi(portToken));
		token = strtok(NULL,"-");

	}
	/*
	//imprime lista de servidores (teste) 
	struct serverList *pointer = serverList;
	struct serverList *anotherPointer = serverList;
		do{
		fprintf(stderr,"%s - %d isPrimary:%d - Previous: %s\n", pointer->serverName, pointer->port, isPrimary(pointer->serverName,pointer->port,&serverList), previousServer(pointer->serverName,&serverList));
		
		pointer=pointer->next;

	}while(pointer!= anotherPointer);
 	*/
	//abre conexão replica
	
	
	while(!isPrimary(ip,myPORT,&serverList)){
		primaryServerNode = primaryServer(&serverList);
		secondaryServer(primaryServerNode->serverName,primaryServerNode->port);
	}
		


	if(pthread_create(&thread_replica, NULL, createServerPrimary, NULL) < 0){
		fprintf(stderr,"ERROR, could not create thread.\n");
		exit(-1);
	}

	printf("Opening Socket...\n");
	//socket para conexão de clientes
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr,"ERROR opening socket.\n");
		exit(-1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
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
