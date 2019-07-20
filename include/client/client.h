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
#define MAXNAME 64
#include "../../include/common/common.h"

extern pthread_mutex_t clientMutex;

extern pthread_mutex_t writeListenMutex;

extern sem_t inotifySemaphore;

extern sem_t writerSemaphore;

struct inotyClient{
  char userName[MAXNAME];
  int socket;
};

char lastFile[FILENAME_SIZE];
/*
  Listener do cliente que recebe todos os pacotes que inicializam uma operação com o servidor e decide o que fazer de acordo com o pacote
*/
void *listener(void *socket);

/*
  Faz a sincronização inicial do servidor
*/
void synchronize(int sockfd,char* clientName);
/*
  Recebe o retorno do servidor do list_server e imprime na tela
*/
void clientListServer(int sockfd);

/*
  Faz o download dos arquivos que estão na pasta do servidor
*/
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