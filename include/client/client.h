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

#define SERVER_IP_SIZE 64
#define SERVER_PORT_SIZE 12


extern pthread_mutex_t clientMutex;

extern pthread_mutex_t writeListenMutex;

extern sem_t inotifySemaphore;

extern sem_t writerSemaphore;

extern int serverSockfd;

extern int exitCommand;

struct inotyClient{
  char userName[64];
  int socket;
};


char lastFile[FILENAME_SIZE];
/*
  Listener do cliente que recebe todos os pacotes que inicializam uma operação com o servidor e decide o que fazer de acordo com o pacote
*/
void *listener();

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
  Verifica se é um arquivo temporário do gedit
*/
int checkTemp(char* eventName);
/*
  Se o semaforo está em zero, dá post. Usado para controle de fluxo
*/
void checkAndPost(sem_t *semaphore);
/*
 Inicializa Semaforos
*/
void semInit();

/*
  Se conecta ao servidor, modifica o socket global serverSockfd
*/
void connectToServer(char* serverIp, char* serverPort);
/*
  Verifica se está autorizado a conectar no servidor
 */
int authorization(char* userName);

/*
 Escreve comandos e envia ao servidor
*/
void *writer(void* name);

/*
  FrontEnd do cliente
*/
void frontEnd(char* userName, char* serverIp, char* serverPort);

/*
  Abre a porta 4070 do cliente para o envio de mensagem de um novo servidor primario, e recebe o IP e porta do novo primário por meio de reads
*/
void *serverReconnection();

/*
  Retorna no buffer 'ip' o IP desejado ("eth0"....).
*/
void myIp(char* wantedIP, char* ip);

#endif