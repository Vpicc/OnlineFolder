#ifndef __common__
#define __common__
/**
 * 
 * 
 * Colocar structs, defines comuns a clientes e servidor aqui
 * 
 * 
 * */

#define PORT 4000

#define TRUE 1

#define FALSE 0

#define WAITING 2

#define PAYLOAD_SIZE 512

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#define CLIENT_NAME_SIZE 64

#define FILENAME_SIZE 256

#define TYPE_UPLOAD 10

#define TYPE_UPLOAD_READY 11

#define TYPE_DOWNLOAD 20

#define TYPE_DOWNLOAD_READY 21

#define TYPE_DATA 01

#define TYPE_DELETE 30

#define TYPE_EXIT 90

#define TYPE_LIST_SERVER 40

#define TYPE_LIST_SERVER_READY 41

#define TYPE_LIST_CLIENT 50

#define  TYPE_GET_SYNC_DIR 60

#define  TYPE_GET_SYNC_DIR_READY 61

#define TYPE_INOTIFY 70

#define TYPE_INOTIFY_DELETE 75

#define PACKET_SIZE (sizeof (struct packet))


typedef struct packet {
    uint16_t type; // Tipo do pacote ( DATA | CMD )
    uint16_t seqn; // Numero de sequencia
    uint16_t length; // Comprimento do payload
    uint32_t total_size; // Numero total de fragmentos
    char clientName[CLIENT_NAME_SIZE];
    char fileName[FILENAME_SIZE];
    char _payload[PAYLOAD_SIZE]; // Dados do pacote
} packet;


// Global entre listener e client
char clientPath[768];

void serializePacket(packet* inPacket, char* serialized);

void deserializePacket(packet* outPacket, char* serialized);

/**
 * Faz o upload de uma stream de TYPE_DATA
 * */
void upload(int sockfd, char* path, char* clientName, int server);

/**
 * Manda um Packet do TYPE_UPLOAD, depois envia usando a funcao upload
 * */
void uploadCommand(int sockfd, char* path, char* clientName, int server);

/**
 * Faz o download de uma stream de TYPE_DATA
 * */
void download(int sockfd, char* fileName, char* clientName, int server);

/**
 * Manda um Packet do TYPE_DOWNLOAD, depois fica pronto para receber uma stream TYPE_DATA usando a funcao download
 * */
void downloadCommand(int sockfd, char* path, char* clientName, int server);

void deleteCommand(int sockfd,char *path, char *clientName);

void delete(int sockfd, char* fileName, char* pathUser);

void list_serverCommand(int sockfd, char *clientName);

void list_files(int sockfd,char *pathToUser, int server);

void list_clientCommand(int sockfd, char *clientName);

void inotifyUpCommand(int sockfd, char* path, char* clientName, int server);

/*
  Packet do TYPE_GET_SYNC_DIR
*/
void getSyncDirCommand(int sockfd, char* clientName);

/*
  Lança uma thread para ficar no watcher no path de argumento
*/
void *inotifyWatcher(void *pathToWatch);

/*
  Verifica se o usuario já tem o diretorio criado na pasta User com seu nome, e se não tiver já o cria
*/
int checkAndCreateDir(char *pathName);
/*
 Retorna o nome do arquivo a partir do path
*/
char* getFileName(char *path);
/*
 Atribui valores ao packet 
*/
void setPacket(packet *packetToSet,int type, int seqn, int length, int total_size, char* fileName, char* clientName, char* payload);
/*
 Atribui caminho relativo ate arquivo
*/
char* pathToFile(char* pathUser, char* fileName);

/*
 Envia um packet falando que está prondo para baixar
*/

void readyToDownload(int sockfd, char* fileName, char* clientName);
/*
 Envia um packet falando que está prondo para Listar arquivos do servidor
*/
void readyToListServer(int sockfd);

/*
 Envia um packet falando que está prondo para dar Upload
*/

void readyToUpload(int sockfd, char* fileName, char* clientName);

/*
 Envia um packet falando que está prondo para dar sync_dir
*/

void readyToSyncDir(int sockfd, char* clientName);

/*
 Recebe FileNames de uma pasta e faz o upload de cada uma
*/
void uploadAll(int sockfd,char *pathToUser);

void inotifyDelCommand(int sockfd, char *path, char *clientName);



#endif