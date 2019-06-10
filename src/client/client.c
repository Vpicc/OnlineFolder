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
#include <semaphore.h>
#include <errno.h>
#include <sys/inotify.h>
#include <dirent.h>

#include "../../include/linkedlist/linkedlist.h"
#include "../../include/client/client.h"


pthread_mutex_t listenerInotifyMut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t noListen = PTHREAD_COND_INITIALIZER, noInotify = PTHREAD_COND_INITIALIZER;

int synching = FALSE;

int inotifyLength = 0;
int listenerStatus = 0;

int serverSockfd;

void *listener(){
    char response[PACKET_SIZE];
    packet incomingPacket;
    bzero(response, PACKET_SIZE);

    while(1){
        
        listenerStatus = read(serverSockfd, response, PACKET_SIZE);
        
        deserializePacket(&incomingPacket,response);
        

        if(listenerStatus == 0) {
            printf("\nConnection Closed.\n");
            exit(-1);
        }
        
        pthread_mutex_lock(&listenerInotifyMut);
        //printf("PACKET: %u %u %u %u %s %s %s\n", incomingPacket.type,incomingPacket.seqn,incomingPacket.length,incomingPacket.total_size,incomingPacket.clientName,incomingPacket.fileName,incomingPacket._payload);
        while(inotifyLength) {
            pthread_cond_wait(&noListen, &listenerInotifyMut);
        }
        listenerStatus = 0;
        pthread_mutex_lock(&clientMutex);
        switch(incomingPacket.type) {
                case TYPE_UPLOAD:
                    printf("\nDownloading %s...\n", incomingPacket.fileName);
                    download(serverSockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    printf("\n%s Downloaded.\n", incomingPacket.fileName);
                    break;
                case TYPE_MIRROR_UPLOAD:
                    strcpy(lastFile,incomingPacket.fileName);
                    downloadCommand(serverSockfd,incomingPacket.fileName,incomingPacket.clientName,FALSE);
                    read(serverSockfd, response, PACKET_SIZE);
                    deserializePacket(&incomingPacket,response);
                    if(incomingPacket.type == TYPE_UPLOAD_READY){
                        printf("\nDownloading %s...\n", incomingPacket.fileName);
                        download(serverSockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                        printf("\n%s Downloaded.\n", incomingPacket.fileName);
                    }
                    break;
                case TYPE_INOTIFY_DELETE:
                    strcpy(lastFile,incomingPacket.fileName);
                    printf("\nDeleting %s...\n", incomingPacket.fileName);
                    delete(serverSockfd,incomingPacket.fileName, incomingPacket.clientName);   
                    break;
                case TYPE_DOWNLOAD_READY:
                    printf("\nUploading %s...\n", incomingPacket.fileName);
                    upload(serverSockfd,clientPath,incomingPacket.clientName,FALSE);
                    printf("\n%s Uploaded.\n", incomingPacket.fileName);
                    break;
                case TYPE_UPLOAD_READY:
                    printf("\nDownloading %s...\n", incomingPacket.fileName);
                    download(serverSockfd,incomingPacket.fileName,incomingPacket.clientName,FALSE);
                    printf("\n%s Downloaded.\n", incomingPacket.fileName);
                    break;
                case TYPE_LIST_SERVER_READY:
                    clientListServer(serverSockfd);
                    break;
                case TYPE_GET_SYNC_DIR_READY:
                    clientSyncServer(serverSockfd, incomingPacket.clientName);
                    printf("\nAll Files Updated.\n");
                    break;
                case TYPE_INOTIFY_CONFIRMATION:
                    pthread_cond_signal(&noInotify);
                    break;
                default:
                    break;
        }
        pthread_mutex_unlock(&clientMutex);
        pthread_mutex_unlock(&writeListenMutex);
        pthread_cond_signal(&noInotify);
        checkAndPost(&writerSemaphore);
        pthread_mutex_unlock(&listenerInotifyMut);
    }
}

void clientListServer(int sockfd) {
    int status;
    char response[PAYLOAD_SIZE];
    do{
        bzero(response, PAYLOAD_SIZE);
        status = read(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) {
            printf("ERROR reading from socket\n");
            return;
        }

        fprintf(stderr,"%s",response);

        
    } while (strcmp(response,"  ") != 0);
}

void clientSyncServer(int sockfd, char* clientName) {
    int status;
    char response[PAYLOAD_SIZE];
    char buffer[PACKET_SIZE];
    packet incomingPacket;

    synching = TRUE;
    do{
        bzero(response, PAYLOAD_SIZE);
        status = read(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) {
            printf("ERROR reading from socket\n");
            return;
        }
        if(strcmp(response,"  ") != 0) {
            downloadCommand(sockfd,response,clientName,TRUE);
            status = read(sockfd, buffer, PACKET_SIZE);
            deserializePacket(&incomingPacket,buffer);
            if(incomingPacket.type == TYPE_UPLOAD_READY) {
                download(sockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
            } else if(strcmp(buffer,"  ") != 0 ){
                printf("\nERROR Expected Upload Ready Packet\n");
                synching = FALSE;
                return;
            }
        }

        
    } while (strcmp(response,"  ") != 0);

    synching = FALSE;
}

void synchronize(int sockfd,char* clientName) {
    int status;
    char serialized[PACKET_SIZE] = {0};
    packet incomingPacket;
    printf("\nUpdating files...\n");

    getSyncDirCommand(sockfd,clientName);  

    status = read(sockfd,serialized,PACKET_SIZE);
    if (status < 0) {
        printf("ERROR reading from socket\n");
        return;
    }
    deserializePacket(&incomingPacket,serialized);

    if(incomingPacket.type == TYPE_GET_SYNC_DIR_READY) {
        clientSyncServer(sockfd, incomingPacket.clientName);
        printf("\nAll files Updated\n");
    } else {
        printf("\nERROR Expected Get_sync_dir_ready packet\n");
    }
}

void deleteAll(char* clientName) {
    DIR *dir;
    struct dirent *lsdir;
    dir = opendir(clientName);
    char filePath[(CLIENT_NAME_SIZE + FILENAME_SIZE + 1)];
    while ((lsdir = readdir(dir)) != NULL )
    {
        if(strcmp(lsdir->d_name,".") !=0 && strcmp(lsdir->d_name,"..") !=0){
            strcpy(filePath,"");
            strcat(filePath,clientName);
            strcat(filePath,"/");
            strcat(filePath,lsdir->d_name);
            remove(filePath); 
        }
    }
    closedir(dir);
}


void *inotifyWatcher(void *inotifyClient){
    int fd;
    int wd;
    char buffer[BUF_LEN];

    fd = inotify_init();

        if ( fd < 0 ) {
        perror( "inotify_init" );
    }

    wd = inotify_add_watch( fd, ((struct inotyClient*) inotifyClient)->userName, 
                            IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

    while (1) {
        int i = 0;

        inotifyLength = 0;
        inotifyLength = read( fd, buffer, BUF_LEN );
        pthread_mutex_lock(&listenerInotifyMut);

        if ( inotifyLength < 0 ) {
            perror( "read" );
        }
        pthread_mutex_lock(&writeListenMutex);

        if(!synching){   
            while ( i < inotifyLength ) {
                while(listenerStatus) {
                    pthread_cond_wait(&noInotify, &listenerInotifyMut);
                }
                struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
                if ( event->len ) {
                    if(event->name != NULL) {
                        strcpy(clientPath, ((struct inotyClient*) inotifyClient)->userName);
                        strcat(clientPath,"/");
                        strcat(clientPath,event->name);
                    }
                    if(!checkTemp(event->name)){
                        if ((event->mask & IN_CLOSE_WRITE)){
                            if(strcmp(event->name,lastFile)!=0){
                                //cria o caminho: username/file
                                printf( "\nThe file %s was created in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                                inotifyUpCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName, TRUE);
                                bzero(lastFile,FILENAME_SIZE);      
                            }
                            else{
                                printf("\nNão precisa ativar o Inotify\n");
                                bzero(lastFile,FILENAME_SIZE);
                            }
                        } else if (event->mask & IN_MOVED_TO) {
                        
                            if(strcmp(event->name,lastFile)!=0){
                                //cria o caminho: username/file
                                printf( "\nThe file %s was created in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                                inotifyUpCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName, TRUE);
                                bzero(lastFile,FILENAME_SIZE);      
                            }
                            else{
                                printf("\nNão precisa ativar o Inotify\n");
                                bzero(lastFile,FILENAME_SIZE);
                            }
                        }
                        else if ( event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                            if(strcmp(event->name,lastFile)!=0){
                                //cria o caminho: username/file
                                printf( "\nThe file %s was deleted in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                                inotifyDelCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName);
                                bzero(lastFile,FILENAME_SIZE);
                            }
                            else{
                                printf("\nNão precisa ativar o Inotify\n");
                                bzero(lastFile,FILENAME_SIZE);
                            }
                                
                        }
                    } else {
                    }
                }
                i += EVENT_SIZE + event->len;
                pthread_cond_signal(&noListen);
            }
            
        }
        pthread_mutex_unlock(&writeListenMutex);
        pthread_mutex_unlock(&listenerInotifyMut);

    }

    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );
}

int checkTemp(char* eventName) {
    char fileName[FILENAME_SIZE];
    char* firstHalf;
    strcpy(fileName,eventName);
    firstHalf = strtok(fileName,"-");
    if(firstHalf != NULL){
        if (strcmp(firstHalf,".goutputstream") == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

void checkAndPost(sem_t *semaphore) {
    int semStatus;
    sem_getvalue(semaphore,&semStatus);
    if (semStatus == 0) {
        sem_post(semaphore);
    }
}

void semInit() {
    sem_init(&writerSemaphore,0,0);
}

void connectToServer(char* serverIp, char* serverPort) {
    struct hostent *server;
    struct sockaddr_in serv_addr;
    
    server = gethostbyname(serverIp);

	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    
    if ((serverSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(atoi(serverPort));
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
	bzero(&(serv_addr.sin_zero), 8);

    
	if (connect(serverSockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(-1);
    }

}

int authorization(char* userName) {
    int authorization = WAITING;
    char buffer[PAYLOAD_SIZE] = {0};
    int idUserName;
    char response[PACKET_SIZE] = {0};

    sprintf(buffer,"%s",userName);
    idUserName = write(serverSockfd, buffer, PAYLOAD_SIZE);
    // envia o username para o servidor
    if (idUserName < 0) 
        printf("ERROR writing to socket\n");
    /*
    Espera autorização do servidor para validar a conexão
    */
    while(authorization == WAITING) {
        read(serverSockfd, response, PACKET_SIZE);
        if(strcmp(response,"authorized") == 0){
            authorization = TRUE;
        } else if(strcmp(response,"notauthorized") == 0) {
            printf("There is a connection limit of up to two connected devices.\n");
            authorization = FALSE;
            exitCommand = TRUE;
        }
    }
    return authorization;    
}

void *writer(void* name) {
    char command[PAYLOAD_SIZE] = {0};
    char *option;
    char *path;
    char *fileName;
    int error;
    char *userName = (char*)name;


    printf("\nConsole Started, you can now enter commands.\n");

    while (exitCommand == FALSE) {

        bzero(command, PAYLOAD_SIZE);
        fflush(stdin);
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
        pthread_mutex_lock(&writeListenMutex);

        // Switch for options
        if(strcmp(option,"exit") == 0) {
            exitCommand = TRUE;
            error = ERRORCODE;
        } else if (strcmp(option, "upload") == 0) { // upload from path
            fileName = strrchr(path,'/');
            if(fileName != NULL){
                fileName++;
            } 
            else {
                fileName = path;
            }
            strcpy(lastFile, fileName);
            error = uploadCommand(serverSockfd,path,userName, FALSE);          
        } else if (strcmp(option, "download") == 0) { // download to exec folder
            error = downloadCommand(serverSockfd,path,userName, FALSE);
        } else if (strcmp(option, "delete") == 0) { // delete from syncd dir
            fileName = strrchr(path,'/');
            if(fileName != NULL){
                fileName++;
            } 
            else {
                fileName = path;
            }
            strcpy(lastFile, fileName);
            error = deleteCommand(serverSockfd,path,userName);
        } else if (strcmp(option, "list_server") == 0) { // list user's saved files on dir
            error = list_serverCommand(serverSockfd,userName);
        } else if (strcmp(option, "list_client") == 0) { // list saved files on dir
            error = list_clientCommand(serverSockfd,userName);
        } else if (strcmp(option, "get_sync_dir") == 0) { // creates sync_dir_<username> and syncs
            error = checkAndCreateDir(userName);
            error = getSyncDirCommand(serverSockfd,userName);            
        } else {
            printf("\nInvalid Command.\n");
            error = ERRORCODE;
        }
        pthread_mutex_unlock(&clientMutex);
        if((strcmp(option, "list_client") != 0 && error != ERRORCODE)){
            sem_wait(&writerSemaphore);
        }else{
            pthread_mutex_unlock(&writeListenMutex);
        }

    }
    return NULL;
}


void frontEnd(char* userName, char* serverIp, char* serverPort) {
    int initialization = 1;
    pthread_t thread_inotify, thread_listener, thread_writer;
    struct inotyClient *inotyClient = malloc(sizeof(*inotyClient));

    if(initialization){
        connectToServer(serverIp,serverPort);
        initialization = 0;
    } else {

    }

    if(authorization(userName) == TRUE) {
        inotyClient->socket = serverSockfd;
        strcpy(inotyClient->userName, userName);
        checkAndCreateDir(userName);
        deleteAll(userName);
        synchronize(serverSockfd,userName);
        
        if(pthread_create(&thread_inotify, NULL, inotifyWatcher, (void *) inotyClient) < 0){ // Inotify
            fprintf(stderr,"ERROR, could not create inotify thread.\n");
            exit(-1);
        }
        if(pthread_create(&thread_listener, NULL, listener, NULL) < 0){ // Updates from server
            fprintf(stderr,"ERROR, could not create listener thread.\n");
            exit(-1);
        }
        if(pthread_create(&thread_writer, NULL, writer, (void *) userName) < 0){ // Inotify
            fprintf(stderr,"ERROR, could not create inotify thread.\n");
            exit(-1);
        }

        pthread_join(thread_writer,NULL);

    }
    

}

