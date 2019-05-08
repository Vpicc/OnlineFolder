#define MAXNAME 64
#define MAXFILES 20
#define FREEDEV -1

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


#define INSERTDEVICE 0
#define REMOVEDEVICE 1

#define SUCESS 1


struct clientList *clientList; //Inicialização do nodo inicial da lista de clientes


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

struct client
{
  int devices[2];
  char userName[MAXNAME];
};

struct clientList
{
  struct client client;
  struct clientList *next;
};