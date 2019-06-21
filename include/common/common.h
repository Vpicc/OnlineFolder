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

#define RECONNECTION_PORT 4070

#define SERVERPORT 4010

#define ERRORCODE -1

#define SUCCESS 0

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

#define TYPE_MIRROR_UPLOAD 12

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

#define TYPE_INOTIFY_CONFIRMATION 72

#define TYPE_INOTIFY_DELETE 75

#define PACKET_SIZE (sizeof (struct packet))

#define WANTED_IP "enp3s0"


typedef struct packet {
    uint16_t type; // Tipo do pacote ( DATA | CMD )
    uint32_t seqn; // Numero de sequencia
    uint16_t length; // Comprimento do payload
    uint32_t total_size; // Numero total de fragmentos
    char clientName[CLIENT_NAME_SIZE];
    char fileName[FILENAME_SIZE];
    char _payload[PAYLOAD_SIZE]; // Dados do pacote
} packet;


// Global entre listener e client
char clientPath[768];

/*
  Serializa um packet para um array de chars
*/
void serializePacket(packet* inPacket, char* serialized);
/*
  Desserializa um array de chars para um packet
*/
void deserializePacket(packet* outPacket, char* serialized);

/**
 * Faz o upload de uma stream de TYPE_DATA
 * */
void upload(int sockfd, char* path, char* clientName, int server);

/**
 * Manda um Packet do TYPE_UPLOAD
 * */
int uploadCommand(int sockfd, char* path, char* clientName, int server);

/**
 * Faz o download de uma stream de TYPE_DATA
 * */
void download(int sockfd, char* fileName, char* clientName, int server);

/**
 * Manda um Packet do TYPE_DOWNLOAD
 * */
int downloadCommand(int sockfd, char* path, char* clientName, int server);

/*
  Envia um packet do TYPE_DELETE
*/
int deleteCommand(int sockfd,char *path, char *clientName);

/*
  Deleta o arquivo
*/
void delete(int sockfd, char* fileName, char* pathUser);

/*
  Envia um packet do TYPE_LIST_SERVER
*/
int list_serverCommand(int sockfd, char *clientName);

/*
  Lista os arquivos se for cliente, se for servidor, envia a lista de arquivos
*/
void list_files(int sockfd,char *pathToUser, int server);

/*
  Lista os arquivos
*/
int list_clientCommand(int sockfd, char *clientName);

/*
  Manda um packet falando que um arquivo apareceu na pasta e está pronto para upload
*/
void inotifyUpCommand(int sockfd, char* path, char* clientName, int server);

/*
  Packet do TYPE_GET_SYNC_DIR
*/
int getSyncDirCommand(int sockfd, char* clientName);

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

/*
  Envia um packet falando que um arquivo foi deletado na pasta do cliente
*/
void inotifyDelCommand(int sockfd, char *path, char *clientName);

/*
 Espelha um upload
*/
void mirrorUploadCommand(int sockfd, char *path, char *clientName);
/*
  Confirma que o servidor fez a ação do inotify
*/
void inotifyConfirmation(int sockfd, char *path, char *clientName);
/*
  Retorna no buffer 'ip' o IP desejado ("eth0"....).
*/
void myIp(char* wantedIP, char* ip);
/*
  Connect to server -- retorna socket
*/
int connectToServerTest(char* serverIp, char* serverPort);

#endif