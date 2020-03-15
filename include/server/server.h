#ifndef __server__
#define __server__
#include <pthread.h>
#define MAXNAME 64
#define MAXFILES 20
#define FREEDEV -1

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


#define INSERTDEVICE 0
#define REMOVEDEVICE 1

#define SUCESS 1


struct clientList *clientList; //Inicialização do nodo inicial da lista de clientes

/*
  Função que escuta a nova conexão de cliente e faz dispatch dos comandos de acordo com o packet recebido
*/
void *handleConnection(void *socketDescriptor);
/*
  Adiciona um novo cliente na lista de clientes
*/
void appendNewClient(int socketNewClient, char* userName);
/*
  Atualiza o numero de devices de um cliente na lista.
  colocar em option INSERTDEVICE(0) para inserir e REMOVE DEVICE(1) para remover
*/
int updateNumberOfDevices(struct clientList *client_node, int socketNumber, int option);

/*
  Procura um segundo socket associado a um cliente
*/
int otherSocketDevice (char *userName, int actSocket);

struct client
{
  int devices[2];
  char userName[MAXNAME];
  pthread_mutex_t clientPairMutex;
};

struct clientList
{
  struct client client;
  struct clientList *next;
};

#endif