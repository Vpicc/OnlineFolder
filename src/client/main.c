#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include "../../include/client/client.h"
#include "../../include/common/common.h"


pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;

int inotifyInitialized = FALSE;

int main(int argc, char *argv[])
{
    int sockfd;
    int authorization = WAITING;
    int exitCommand = FALSE;
    char command[PAYLOAD_SIZE];
    char response[PAYLOAD_SIZE];
    char buffer[PAYLOAD_SIZE] = {0};
    char *option;
    char *path;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int idUserName;
    char *fileName;
    pthread_t thread_id, thread_id2;

    bzero(command,PAYLOAD_SIZE);

    if (argc < 3) {
		fprintf(stderr,"usage %s clientname hostname\n", argv[0]);
		exit(-1);
    }
	
	server = gethostbyname(argv[2]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    /*Test Lista*/

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
	bzero(&(serv_addr.sin_zero), 8);

    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(-1);
    }

    // Inicia semaforos
    semInit();


    sprintf(buffer,"%s",argv[1]);
    idUserName = write(sockfd, buffer, PAYLOAD_SIZE);
    // envia o username para o servidor
    if (idUserName < 0) 
        printf("ERROR writing to socket\n");
    /*
    Espera autorização do servidor para validar a conexão
    */
    struct inotyClient *inotyClient = malloc(sizeof(*inotyClient));
    inotyClient->socket = sockfd;
    strcpy(inotyClient->userName, argv[1]);
    while(authorization == WAITING){
        read(sockfd, response, PACKET_SIZE);
        if(strcmp(response,"authorized") == 0){
            // get_sync_dir
            checkAndCreateDir(argv[1]);
            deleteAll(argv[1]);
            synchronize(sockfd,argv[1]);
            //
            if(pthread_create(&thread_id, NULL, inotifyWatcher, (void *) inotyClient) < 0){ // Inotify
			    fprintf(stderr,"ERROR, could not create thread.\n");
			    exit(-1);
		    }
            if(pthread_create(&thread_id2, NULL, listener, (void *) &sockfd) < 0){ // Updates from server
			    fprintf(stderr,"ERROR, could not create thread.\n");
			    exit(-1);
		    }
            authorization = TRUE;

        }
        if(strcmp(response,"notauthorized") == 0){
            printf("There is a connection limit of up to two connected devices.\n");
            authorization = FALSE;
            exitCommand = TRUE;
        }
    }


    while (exitCommand == FALSE) {

        printf("\nEnter the Command: ");
        bzero(command, PAYLOAD_SIZE);
        fgets(command, PAYLOAD_SIZE, stdin);
        if(strcspn(command, "\n")>0)
            command[strcspn(command, "\n")] = 0;

        option = strtok(command," ");
        path = strtok(NULL," ");
        if(path != NULL) {
            path = strtok(path,"\n");
        }

        printf("OPTION: %s\n", option);
        printf("PATH: %s\n", path);

        // Stores clientPath for listener
        if(path != NULL) {
            sprintf(clientPath,"%s",path);
        }
        
        pthread_mutex_lock(&clientMutex);
        // Switch for options
        if(strcmp(option,"exit") == 0) {
            exitCommand = TRUE;
        } else if (strcmp(option, "upload") == 0) { // upload from path
            fileName = strrchr(path,'/');
            if(fileName != NULL){
                fileName++;
            } 
            else {
                fileName = path;
            }
            strcpy(lastFile, fileName);
            uploadCommand(sockfd,path,argv[1], FALSE);          
        } else if (strcmp(option, "download") == 0) { // download to exec folder
            downloadCommand(sockfd,path,argv[1], FALSE);
        } else if (strcmp(option, "delete") == 0) { // delete from syncd dir
        fileName = strrchr(path,'/');
            if(fileName != NULL){
                fileName++;
            } 
            else {
                fileName = path;
            }
            strcpy(lastFile, fileName);
            deleteCommand(sockfd,path,argv[1]);
        } else if (strcmp(option, "list_server") == 0) { // list user's saved files on dir
            list_serverCommand(sockfd,argv[1]);
        } else if (strcmp(option, "list_client") == 0) { // list saved files on dir
            list_clientCommand(sockfd,argv[1]);
        } else if (strcmp(option, "get_sync_dir") == 0) { // creates sync_dir_<username> and syncs
            checkAndCreateDir(argv[1]);
            getSyncDirCommand(sockfd,argv[1]);            
        }
        pthread_mutex_unlock(&clientMutex);
    }

	close(sockfd);

    printf("Connection Closed. Exiting Program.\n");
    return 0;
}