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

int inotifyInAction = FALSE;

sem_t inotifySemaphore;

sem_t listenerSemaphore;

int synching = FALSE;

void *listener(void *socket){
    int semStatus;
    //char command[PAYLOAD_SIZE];
    char response[PACKET_SIZE];
    int status;
    int connectionSocket = *(int*)socket;
    packet incomingPacket;
    bzero(response, PACKET_SIZE);

    while(1){
        status = read(connectionSocket, response, PACKET_SIZE);
        deserializePacket(&incomingPacket,response);
        strcpy(lastFile,incomingPacket.fileName);

        if(status == 0) {
            printf("\nConnection Closed.\n");
            exit(-1);
        }

        //printf("PACKET: %u %u %u %u %s %s %s\n", incomingPacket.type,incomingPacket.seqn,incomingPacket.length,incomingPacket.total_size,incomingPacket.clientName,incomingPacket.fileName,incomingPacket._payload);
        if(inotifyInAction) {
            sem_wait(&listenerSemaphore);
        }
        pthread_mutex_lock(&clientMutex);
        switch(incomingPacket.type) {
                case TYPE_UPLOAD:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    bzero(lastFile,FILENAME_SIZE);
                    break;
                case TYPE_INOTIFY:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    bzero(lastFile,FILENAME_SIZE);                    
                    break;
                case TYPE_DELETE:
                    delete(connectionSocket,incomingPacket.fileName, incomingPacket.clientName);
                    bzero(lastFile,FILENAME_SIZE);                    
                    break;
                case TYPE_INOTIFY_DELETE:
                    delete(connectionSocket,incomingPacket.fileName, incomingPacket.clientName);
                    bzero(lastFile,FILENAME_SIZE);                    
                    break;
                case TYPE_DOWNLOAD_READY:
                    upload(connectionSocket,clientPath,incomingPacket.clientName,FALSE);
                    bzero(lastFile,FILENAME_SIZE);  
                    break;
                case TYPE_UPLOAD_READY:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,FALSE);
                    bzero(lastFile,FILENAME_SIZE);  
                    break;
                case TYPE_LIST_SERVER_READY:
                    clientListServer(connectionSocket);
                    bzero(lastFile,FILENAME_SIZE);  
                    break;
                case TYPE_GET_SYNC_DIR_READY:
                    clientSyncServer(connectionSocket, incomingPacket.clientName);
                    printf("\nAll Files Updated.\n");
                    bzero(lastFile,FILENAME_SIZE);  
                    break;
                default:
                    break;
        }
        pthread_mutex_unlock(&clientMutex);
        sem_getvalue(&inotifySemaphore,&semStatus);
        if (semStatus == 0) {
            sem_post(&inotifySemaphore);
        }
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
    int length;
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
        inotifyInAction = FALSE;
        length = read( fd, buffer, BUF_LEN );

        if ( length < 0 ) {
            perror( "read" );
        }
        
        if(!synching){   
            while ( i < length ) {
                inotifyInAction = TRUE;
                sem_wait(&inotifySemaphore);
                struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
                if ( event->len ) {
                    if(event->name != NULL) {
                        strcpy(clientPath, ((struct inotyClient*) inotifyClient)->userName);
                        strcat(clientPath,"/");
                        strcat(clientPath,event->name);
                    }
                    if ((event->mask & IN_CLOSE_WRITE)){
                        if(strcmp(event->name,lastFile)!=0){
                            //cria o caminho: username/file
                            printf( "\nThe file %s was created in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                            inotifyUpCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName, TRUE);      
                        }
                        else{

                            printf("Não precisa ativar o Inotify\n");
                            bzero(lastFile,FILENAME_SIZE);
                        }
                    } else if (event->mask & IN_MOVED_TO) {
                    
                        if(strcmp(event->name,lastFile)!=0){
                            //cria o caminho: username/file
                            printf( "\nThe file %s was created in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                            inotifyUpCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName, TRUE);      
                        }
                        else{

                            printf("Não precisa ativar o Inotify\n");
                            bzero(lastFile,FILENAME_SIZE);
                        }
                    }
                    else if ( event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                        if(strcmp(event->name,lastFile)!=0){
                            //cria o caminho: username/file
                            printf( "\nThe file %s was deleted in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                            inotifyDelCommand(((struct inotyClient*) inotifyClient)->socket, event->name, ((struct inotyClient*) inotifyClient)->userName);
                            //inotifyDelCommand(((struct inotyClient*) inotifyClient)->socket, ((struct inotyClient*) inotifyClient)->userName ,((struct inotyClient*) inotifyClient)->userName);        
                        }
                        else{
                            printf("Não precisa ativar o Inotify\n");
                            bzero(lastFile,FILENAME_SIZE);
                        }
                            
                    }

                }
                i += EVENT_SIZE + event->len;
                sem_post(&listenerSemaphore);

            }
        }
        
        

    }
    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );
}

void semInit() {
    sem_init(&inotifySemaphore,0,1);
    sem_init(&listenerSemaphore,0,0);
}