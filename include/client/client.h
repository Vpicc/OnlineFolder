#ifndef __client__
#define __client__
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/inotify.h>
#include <semaphore.h>
#include "../../include/common/common.h"

extern pthread_mutex_t clientMutex;

extern sem_t inotifySemaphore; 

struct inotyClient{
  char userName[64];
  int socket;
};

char lastFile[FILENAME_SIZE];

void *listener(void *socket);

void synchronize(int sockfd,char* clientName);

void clientListServer(int sockfd);

void clientSyncServer(int sockfd, char* clientName);

/*
  Deleta todos os arquivos da pasta de um cliente
*/
void deleteAll(char* clientName);
/*
 Inicializa Semaforos
*/
void semInit();

#endif