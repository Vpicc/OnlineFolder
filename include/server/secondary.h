#ifndef __secondary__
#define __secondary__
#include <pthread.h>

#define RING_MAX_LENGTH 3
#define ELECTIONPORT 4099

extern int myPORT;
extern char ip[MAXNAME];
/*
Define funcionamento dos servidores de backup
*/
void secondaryServer(char *primaryServerIp, int primaryServerPort);
/*
Envia elect para o proximo do anel
*/
void sendElectToNext(int sockfd,int serverNumber);
/*
Inicia processo de eleicao
*/
void election();
/*
Envia elected para o proximo do anel
*/
void sendElectedtoNext(int sockfd, int serverNumber);
/*
Le mensagens elect e elected
*/
void readElectAndElected(int sockfd);

void createServerRing();

void *listenFromRing(void *socketDescriptor);

void *writeToRing();
#endif