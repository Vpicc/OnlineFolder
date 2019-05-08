#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <time.h>
#include "../../include/common/common.h"

void serializePacket(packet* inPacket, char* serialized) {
    uint16_t* buffer16 = (uint16_t*) serialized;
    uint32_t* buffer32;
    char* buffer;
    int i = 0;

    *buffer16 = htons(inPacket->type);
    buffer16++;
    *buffer16 = htons(inPacket->seqn);
    buffer16++;
    *buffer16 = htons(inPacket->length);
    buffer16++;
    buffer32 = (uint32_t*) buffer16;
    *buffer32 = htonl(inPacket->total_size);
    buffer32++;
    buffer = (char*)buffer32;

    for(i = 0; i < CLIENT_NAME_SIZE; i++) {
        *buffer = inPacket->clientName[i];
        buffer++;
    }

    for(i = 0; i < FILENAME_SIZE; i++) {
        *buffer = inPacket->fileName[i];
        buffer++;
    }

    for(i = 0; i < PAYLOAD_SIZE; i++) {
        *buffer = inPacket->_payload[i];
        buffer++;
    }

    return;
}


void deserializePacket(packet* outPacket, char* serialized) {
    uint16_t* buffer16 = (uint16_t*) serialized;
    uint32_t* buffer32;
    char* buffer;
    int i = 0;

    outPacket->type = ntohs(*buffer16);
    buffer16++;
    outPacket->seqn = ntohs(*buffer16);
    buffer16++;
    outPacket->length = ntohs(*buffer16);
    buffer16++;
    buffer32 = (uint32_t*)buffer16;
    outPacket->total_size = ntohl(*buffer32);
    buffer32++;
    buffer = (char*)buffer32;

    for(i = 0; i < CLIENT_NAME_SIZE; i++) {
        outPacket->clientName[i] = *buffer;
        buffer++;
    }

    for(i = 0; i < FILENAME_SIZE; i++) {
        outPacket->fileName[i] = *buffer;
        buffer++;
    }

    for(i = 0; i < PAYLOAD_SIZE; i++) {
        outPacket->_payload[i] = *buffer;
        buffer++;
    }


    return;
}

void uploadCommand(int sockfd, char* path, char* clientName, int server) {
    int status;
    int fileSize;
    int i = 0;
    char buffer[PAYLOAD_SIZE] = {0};
    char* fileName;
    char* finalPath = malloc(strlen(path) + strlen(clientName) + 11);
    uint16_t nread = 0;
    uint32_t totalSize;
    FILE *fp;
    char response[PAYLOAD_SIZE];
    char serialized[PACKET_SIZE];
    packet packetToUpload;

    // Pega o nome do arquivo a partir do path
    fileName = strrchr(path,'/');
    if(fileName != NULL){
        fileName++;
    } else {
        fileName = path;
    }

    if(server) {
        strcpy(finalPath,"");
        strcat(finalPath,clientName);
        strcat(finalPath,"/");
        strcat(finalPath,fileName);
    } else {
        strncpy(finalPath,path,strlen(path)+1);
    }

    // Pega o tamanho do arquivo
    fp = fopen(finalPath,"r");
    if(fp == NULL) {
        printf("ERROR Could not read file.\n");
        return;
    }
    fseek(fp,0,SEEK_END);
    fileSize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    totalSize = fileSize / PAYLOAD_SIZE;

    packetToUpload.type = TYPE_UPLOAD;
    packetToUpload.seqn = i;
    packetToUpload.length = nread;
    packetToUpload.total_size = totalSize;
    strncpy(packetToUpload.fileName,fileName,FILENAME_SIZE);
    strncpy(packetToUpload.clientName,clientName,CLIENT_NAME_SIZE);
    strncpy(packetToUpload._payload,buffer,PAYLOAD_SIZE);
    
    serializePacket(&packetToUpload,serialized);       
    
    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

    bzero(response, PAYLOAD_SIZE);
    
    /* read from the socket */
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");

    printf("%s\n",response);

    fclose(fp);
    
    upload(sockfd, path, clientName, server);
    
    free(finalPath);
    
}

void upload(int sockfd, char* path, char* clientName, int server) {
    uint16_t nread;
    char buffer[PAYLOAD_SIZE] = {0};
    uint32_t totalSize;
    int fileSize;
    FILE *fp;
    int status;
    char* fileName;
    char* finalPath = malloc(strlen(path) + strlen(clientName) + 11);
    char serialized[PACKET_SIZE];
    char response[PAYLOAD_SIZE];
    packet packetToUpload;
    int i = 0;

    // Pega o nome do arquivo a partir do path
    fileName = strrchr(path,'/');
    if(fileName != NULL){
        fileName++;
    } else {
        fileName = path;
    }

    strncpy(packetToUpload.fileName,fileName,FILENAME_SIZE);
    strncpy(packetToUpload.clientName,clientName,CLIENT_NAME_SIZE);

    if(server) {
        strcpy(finalPath, "");
        strcat(finalPath, clientName);
        strcat(finalPath, "/");
        strcat(finalPath, path);
    }
    else {
        strncpy(finalPath, path, strlen(path) + 1);
    }

    fp = fopen(finalPath,"r");
    if(fp == NULL) {
        printf("ERROR Could not read file.\n");
        return;
    }

    fseek(fp,0,SEEK_END);
    fileSize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    totalSize = fileSize / PAYLOAD_SIZE;

    packetToUpload.total_size = totalSize;

    while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        memset(serialized, '\0', sizeof(serialized));

        packetToUpload.type = TYPE_DATA;
        packetToUpload.seqn = i;
        packetToUpload.length = nread;

        strncpy(packetToUpload._payload, buffer, PAYLOAD_SIZE);

        serializePacket(&packetToUpload, serialized);

        status = write(sockfd, serialized, PACKET_SIZE);

        if(status < 0) {
            printf("ERROR writing to socket\n");
            return;
        }

        bzero(response, PAYLOAD_SIZE);

        /* read from the socket */
        status = read(sockfd, response, PAYLOAD_SIZE);
        if(status < 0) {
            printf("ERROR reading from socket\n");
            return;
        }

        printf("%s\n", response);

        i++;
    }

    fclose(fp);
}


void download(int sockfd, char* fileName, char* clientName, int server) {
    int status;
    int downloaded = 0;
    char *path;
    char serialized[PACKET_SIZE];
    FILE *fp;
    char response[PAYLOAD_SIZE];
    packet packetToDownload;

    if(server) {
        path = malloc(strlen(fileName) + strlen(clientName) + 11); // 11 para por _sync_dir ao final do nome
        strcpy(path,"");
        strcat(path,clientName);
        strcat(path,"/");
        strcat(path,fileName);
        fp = fopen(path,"w");
    } else {
        fp = fopen(fileName,"w");
    }
    if(fp == NULL) {
        printf("ERROR Writing to file");
        return;
    }

    do {
        status = read(sockfd, serialized, PACKET_SIZE);
        if (status <= 0) {
            printf("ERROR reading from socket\n");
            return;
        }

        deserializePacket(&packetToDownload,serialized);

        fwrite(packetToDownload._payload,1,packetToDownload.length,fp);

        downloaded += (int)packetToDownload.length;

        sprintf(response,"%s %u %s","Uploaded ",downloaded, " bytes from file.");

        status = write(sockfd, response, PAYLOAD_SIZE);
        
    } while(packetToDownload.seqn != packetToDownload.total_size);

    if(server) {
        free(path);
    }

    fclose(fp);

}

void downloadCommand(int sockfd, char* path, char* clientName, int server) {
    char* fileName;
    packet packetToDownload;
    char buffer[PAYLOAD_SIZE] = {0};
    char serialized[PACKET_SIZE];
    int status;
    char response[PAYLOAD_SIZE];

        // Pega o nome do arquivo a partir do path
    fileName = strrchr(path,'/');
    if(fileName != NULL){
        fileName++;
    } else {
        fileName = path;
    }

    packetToDownload.type = TYPE_DOWNLOAD;
    packetToDownload.seqn = 0;
    packetToDownload.length = 0;
    packetToDownload.total_size = 0;
    strncpy(packetToDownload.fileName,fileName,FILENAME_SIZE);
    strncpy(packetToDownload.clientName,clientName,CLIENT_NAME_SIZE);
    strncpy(packetToDownload._payload,buffer,PAYLOAD_SIZE);
    
    serializePacket(&packetToDownload,serialized);       
    
    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

    bzero(response, PAYLOAD_SIZE);
    
    /* read from the socket */
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");

    download(sockfd,packetToDownload.fileName,packetToDownload.clientName,FALSE);

}

void inotifyUpCommand(int sockfd, char* path, char* clientName, int server) {
    int status;
    int fileSize;
    int i = 0;
    char buffer[PAYLOAD_SIZE] = {0};
    char* fileName;
    char* finalPath = malloc(strlen(path) + strlen(clientName) + 11);
    uint16_t nread = 0;
    uint32_t totalSize;
    FILE *fp;
    char response[PAYLOAD_SIZE];
    char serialized[PACKET_SIZE];
    packet packetToUpload;

    // Pega o nome do arquivo a partir do path
    fileName = strrchr(path,'/');
    if(fileName != NULL){
        fileName++;
    } else {
        fileName = path;
    }

    if(server) {
        strcpy(finalPath,"");
        strcat(finalPath,clientName);
        strcat(finalPath,"/");
        strcat(finalPath,fileName);
    } else {
        strncpy(finalPath,path,strlen(path)+1);
    }

    // Pega o tamanho do arquivo
    fp = fopen(finalPath,"r");
    if(fp == NULL) {
        printf("ERROR Could not read file.\n");
        return;
    }
    fseek(fp,0,SEEK_END);
    fileSize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    totalSize = fileSize / PAYLOAD_SIZE;

    packetToUpload.type = TYPE_INOTIFY;
    packetToUpload.seqn = i;
    packetToUpload.length = nread;
    packetToUpload.total_size = totalSize;
    strncpy(packetToUpload.fileName,fileName,FILENAME_SIZE);
    strncpy(packetToUpload.clientName,clientName,CLIENT_NAME_SIZE);
    strncpy(packetToUpload._payload,buffer,PAYLOAD_SIZE);
    
    serializePacket(&packetToUpload,serialized);       
    
    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

    bzero(response, PAYLOAD_SIZE);
    
    /* read from the socket */
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");

    printf("%s\n",response);

    fclose(fp);
    
    upload(sockfd, path, clientName, server);
    
    free(finalPath);
    
}

void *inotifyWatcher(void *inotifyClient){
    int length;
    int fd;
    int wd;
    char buffer[BUF_LEN];
    char pathComplet[500];
    bzero(pathComplet,500);

    fd = inotify_init();

        if ( fd < 0 ) {
        perror( "inotify_init" );
    }

    wd = inotify_add_watch( fd, ((struct inotyClient*) inotifyClient)->userName, 
                            IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

     while (1) {
        int i = 0;
        length = read( fd, buffer, BUF_LEN );

        if ( length < 0 ) {
            perror( "read" );
        }  

        while ( i < length ) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) {
                if ( event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
                    if(strcmp(event->name,lastFile)!=0){
                            //cria o caminho: username/file
                            strcpy(pathComplet,((struct inotyClient*) inotifyClient)->userName );
                            strcat(pathComplet,"/");
                            strcat(pathComplet, event->name);
                            printf( "\nThe file %s was created in %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);
                            inotifyUpCommand(((struct inotyClient*) inotifyClient)->socket, pathComplet ,((struct inotyClient*) inotifyClient)->userName, FALSE);        
                        }
                        else{
                            bzero(lastFile,100);
                        }
                }
                else if ( event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                    printf( "Remover o arquivo %s do servidor na pasta do %s.\n", event->name,((struct inotyClient*) inotifyClient)->userName);    
                    }
                    
            }
            i += EVENT_SIZE + event->len;
        }
    }
    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );
}


int checkAndCreateDir(char *pathName){
    struct stat sb;
    //printf("%s",strcat(pathcomplete, userName));
    if (stat(pathName, &sb) == 0 && S_ISDIR(sb.st_mode)){
        // usuário já tem o diretório com o seu nome
        return 0;
    }
    else{
        if (mkdir(pathName, 0777) < 0){
            //....
            return -1;
        }
        // diretório não existe
        else{
            printf("Creating %s directory...\n", pathName);
            return 0;
        }
    }
}
void deleteCommand(int sockfd, char *path, char *clientName){
    
    char* fileName;
    char serialized[PACKET_SIZE];
    packet packetToDelete;
    int status;
    char response[PAYLOAD_SIZE];

    fileName = getFileName(path);

    setPacket(&packetToDelete,TYPE_DELETE,0,0,0,fileName,clientName,"");

    serializePacket(&packetToDelete,serialized);

    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

    
    /* captura o executing command */
    bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);
    /* captura a resposta da funcao delete */
    bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);

}
/* Pega o nome do arquivo a partir do path */
char* getFileName(char *path){
    char* fileName;
    fileName = strrchr(path,'/');
    if(fileName != NULL){
        fileName++;
    } else {
        fileName = path;
    }
    return fileName;
}
/* Atribui valores ao packet */
void setPacket(packet *packetToSet,int type, int seqn, int length, int total_size, char* fileName, char* clientName, char* payload){

    packetToSet->type = type;
    packetToSet->seqn = seqn;
    packetToSet->length = length;
    packetToSet->total_size = total_size;
    strncpy(packetToSet->fileName,fileName,FILENAME_SIZE);
    strncpy(packetToSet->clientName,clientName,CLIENT_NAME_SIZE);
    strncpy(packetToSet->_payload,payload,PAYLOAD_SIZE);
}

void delete(int sockfd,char* fileName, char* pathUser){

    int status;
    char response[PAYLOAD_SIZE];
    char filePath[]="";
    bzero(response,PAYLOAD_SIZE);

    pathToFile(filePath,pathUser,fileName);

    if(remove(filePath)==0){
        sprintf(response,"%s deleted sucessfully",fileName);
        status = write(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR writing to socket\n");
    }else{
        sprintf(response,"%s could not be deleted",fileName);
        status = write(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR writing to socket\n");
    }
        printf("response delete: %s",response);

}

void list_serverCommand(int sockfd, char *clientName){
  
    char serialized[PACKET_SIZE];
    packet packetToListServer;
    int status;
    char response[PAYLOAD_SIZE];

    setPacket(&packetToListServer,TYPE_LIST_SERVER,0,0,0,"",clientName,"");

    serializePacket(&packetToListServer,serialized);


    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");


    do{
        bzero(response, PAYLOAD_SIZE);
        /* read from the socket */
        status = read(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR reading from socket\n");

        fprintf(stderr,"%s",response);
    } while (strcmp(response,"  ") != 0);

}
void pathToFile(char* pathToFile ,char* pathUser, char* fileName){
    
    strcpy(pathToFile,pathUser);
    strcat(pathToFile,"/");
    strcat(pathToFile,fileName);

}
/*Lista arquivos em uma pasta */
void list_files(int sockfd,char *pathToUser, int server){

    struct stat sb;
    char mtime[40];
    char atime[40];
    char ctime[40];
    char filePath[]="";
    int status;
    char response[PAYLOAD_SIZE];
    DIR *dir;
    struct dirent *lsdir;

    bzero(response,PAYLOAD_SIZE);
    dir = opendir(pathToUser);
    /* print all the files within directory */
    while ((lsdir = readdir(dir)) != NULL )
    {

        if(strcmp(lsdir->d_name,".") !=0 && strcmp(lsdir->d_name,"..") !=0){
            bzero(filePath,sizeof(filePath));
            pathToFile(filePath,pathToUser,lsdir->d_name);
            stat(filePath, &sb);
            strftime(ctime, 40, "%c", localtime(&(sb.st_ctime)));
            strftime(atime, 40, "%c", localtime(&(sb.st_atime)));
            strftime(mtime, 40, "%c", localtime(&(sb.st_mtime)));
            sprintf(response,"\n%s\n\nmtime: %s\natime: %s\nctime: %s\n\n",lsdir->d_name,mtime ,atime ,ctime);
            if(server){
                status = write(sockfd, response, PAYLOAD_SIZE);
                    if (status < 0) 
                    printf("ERROR writing to socket\n");
            }else{
                printf("%s", response);
            }
        }
    }
    closedir(dir);
    if(server){
        sprintf(response,"  ");
        status = write(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR writing to socket\n");
    }
}
void list_clientCommand(int sockfd, char *clientName){

    list_files(sockfd,clientName,FALSE);

}
