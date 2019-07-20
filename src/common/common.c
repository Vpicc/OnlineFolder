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

int uploadCommand(int sockfd, char* path, char* clientName, int server) {
    int status;
    int fileSize;
    int i = 0;
    char buffer[PAYLOAD_SIZE] = {0};
    char* fileName;
    char* finalPath = malloc(strlen(path) + strlen(clientName) + 11);
    uint16_t nread = 0;
    uint32_t totalSize;
    FILE *fp;
    //char response[PAYLOAD_SIZE];
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
        return ERRORCODE;
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
    if (status < 0){
        printf("ERROR writing to socket\n");
        return ERRORCODE;
    }


    //bzero(response, PAYLOAD_SIZE);
    /* read from the socket */
    /*
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s\n",response);
    */

    fclose(fp);
    
    //upload(sockfd, path, clientName, server);
    
    free(finalPath);
    
    return SUCCESS;
}

void upload(int sockfd, char* path, char* clientName, int server) {
    uint16_t nread = 0;
    char buffer[PAYLOAD_SIZE] = {0};
    char okBuf[3] = {0};
    uint32_t totalSize;
    int fileSize;
    FILE *fp;
    int status;
    char* fileName;
    char* finalPath = malloc(strlen(path) + strlen(clientName) + 11);
    char serialized[PACKET_SIZE];
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

    fp = fopen(finalPath,"rb");
    if(fp == NULL) {
        printf("ERROR Could not read file.\n");
        return;
    }

    fseek(fp,0,SEEK_END);
    fileSize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    totalSize = fileSize / PAYLOAD_SIZE;

    packetToUpload.total_size = totalSize;

    if(fileSize == 0) {
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
    } else {
        while ((nread = fread(buffer, 1, PAYLOAD_SIZE, fp)) > 0) {
            memset(serialized, '\0', sizeof(serialized));

            packetToUpload.type = TYPE_DATA;
            packetToUpload.seqn = i;
            packetToUpload.length = nread;

            memcpy(packetToUpload._payload, buffer, PAYLOAD_SIZE);

            printf("\nPACKET UPLOAD:%u %u %u %u %s %s %s\n",packetToUpload.type, packetToUpload.seqn,packetToUpload.length, packetToUpload.total_size, packetToUpload.clientName, packetToUpload.fileName, packetToUpload._payload);


            serializePacket(&packetToUpload, serialized);

            //do {
                status = write(sockfd, serialized, PACKET_SIZE);
                printf("\nStatus: %d\n",status);

            //} while(!(status < PACKET_SIZE && status != 0));

            if(status < 0) {
                printf("ERROR writing to socket\n");
                return;
            }
            i++;
        }
    }

    fclose(fp);
}


void download(int sockfd, char* fileName, char* clientName, int server) {
    int status;
    int errorFlag;
    char *path;
    char serialized[PACKET_SIZE];
    FILE *fp;
    packet packetToDownload;

    if(server) {
        path = malloc(strlen(fileName) + strlen(clientName) + 11); // 11 para por _sync_dir ao final do nome
        strcpy(path,"");
        strcat(path,clientName);
        strcat(path,"/");
        strcat(path,fileName);
        fp = fopen(path,"wb");
    } else {
        fp = fopen(fileName,"wb");
    }
    if(fp == NULL) {
        printf("ERROR Writing to file");
        return;
    }

    do {
        errorFlag = FALSE;
        //do{
            status = read(sockfd, serialized, PACKET_SIZE);
        //} while(!(status < PACKET_SIZE && status != 0));
        if (status <= 0) {
            printf("ERROR reading from socket\n");
            return;
        } 

        deserializePacket(&packetToDownload,serialized);
        printf("\nPACKET DOWNLOAD:%u %u %u %u %s %s %s\n",packetToDownload.type, packetToDownload.seqn,packetToDownload.length, packetToDownload.total_size, packetToDownload.clientName, packetToDownload.fileName, packetToDownload._payload);

        if(packetToDownload.type == TYPE_DATA) {
            fwrite(packetToDownload._payload,1,packetToDownload.length,fp);
        } else {
            printf("\nERROR expected Packet Type Data, Packet that came instead: %u\n", packetToDownload.type);
            errorFlag = TRUE;
        }

        status = write(sockfd,"OK",3);


    } while(packetToDownload.seqn < packetToDownload.total_size || errorFlag);

    if(server) {
        free(path);
    }

    fclose(fp);

}

int downloadCommand(int sockfd, char* path, char* clientName, int server) {
    char* fileName;
    packet packetToDownload;
    char buffer[PAYLOAD_SIZE] = {0};
    char serialized[PACKET_SIZE];
    int status;
    //char response[PAYLOAD_SIZE];

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
    if (status < 0) {
        printf("ERROR writing to socket\n");
        return ERRORCODE;
    }
    return SUCCESS;
}

int checkAndCreateDir(char *pathName){
    struct stat sb;
    //char synDir[9] = "_sync_dir";
    //printf("%s",strcat(pathcomplete, userName));
    if (stat(pathName, &sb) == 0 && S_ISDIR(sb.st_mode)){
        // usuário já tem o diretório com o seu nome
        return SUCCESS;
    }
    else{
        if (mkdir(pathName, 0777) < 0){
            //....
            return -1;
        }
        // diretório não existe
        else{
            printf("Creating %s directory...\n", pathName);
            return SUCCESS;
        }
    }
}

int deleteCommand(int sockfd, char *path, char *clientName){
    
    char* fileName;
    char serialized[PACKET_SIZE];
    packet packetToDelete;
    int status;
    //char response[PAYLOAD_SIZE];

    fileName = getFileName(path);

    setPacket(&packetToDelete,TYPE_DELETE,0,0,0,fileName,clientName,"");

    serializePacket(&packetToDelete,serialized);

    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0){
        printf("ERROR writing to socket\n");
        return ERRORCODE;
    }
        

    
    /* captura o executing command */
    /*bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);*/
    /* captura a resposta da funcao delete */
    /*bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);*/
    return SUCCESS;

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

void delete(int sockfd, char* fileName, char* pathUser){

    //int status;
    char response[PAYLOAD_SIZE];
    char* filePath;
    bzero(response,PAYLOAD_SIZE);

    filePath = pathToFile(pathUser,fileName);

    if(remove(filePath)==0){
        sprintf(response,"%s deleted sucessfully",fileName);
       /* status = write(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR writing to socket\n");*/
    }else{
        sprintf(response,"%s could not be deleted",fileName);
        /*status = write(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR writing to socket\n");*/

    }
        printf("response delete: %s\n",response);

}

int list_serverCommand(int sockfd, char *clientName){
  
    char serialized[PACKET_SIZE];
    packet packetToListServer;
    int status;
    //char response[PAYLOAD_SIZE];

    setPacket(&packetToListServer,TYPE_LIST_SERVER,0,0,0,"",clientName,"");

    serializePacket(&packetToListServer,serialized);


    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0){
        printf("ERROR writing to socket\n");
        return ERRORCODE;
    }

/*
    do{
        bzero(response, PAYLOAD_SIZE);

        status = read(sockfd, response, PAYLOAD_SIZE);
        if (status < 0) 
            printf("ERROR reading from socket\n");

        fprintf(stderr,"%s",response);
    } while (strcmp(response,"  ") != 0);
*/
return SUCCESS;
}
char* pathToFile(char* pathUser, char* fileName) {
    char* pathToFile;
    pathToFile = malloc(strlen(pathUser) + strlen(fileName) + 1);
    strcpy(pathToFile,pathUser);
    strcat(pathToFile,"/");
    strcat(pathToFile,fileName);
    return pathToFile;
}
/*Lista arquivos em uma pasta */
void list_files(int sockfd,char *pathToUser, int server){

    struct stat sb;
    char mtime[40];
    char atime[40];
    char ctime[40];
    char* filePath;
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
            filePath = pathToFile(pathToUser,lsdir->d_name);
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
            free(filePath);
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
int list_clientCommand(int sockfd, char *clientName){

    list_files(sockfd,clientName,FALSE);

    return SUCCESS;

}

void readyToDownload(int sockfd, char* fileName, char* clientName) {
    packet outPacket;
    int status;
    char serialized[PACKET_SIZE];

    setPacket(&outPacket,TYPE_DOWNLOAD_READY,0,0,0,fileName,clientName,"");

    serializePacket(&outPacket,serialized);
    status = write(sockfd,serialized,PACKET_SIZE);

    if (status < 0) {
        printf("ERROR writing to socket\n");
        return;
    }

}

void readyToUpload(int sockfd, char* fileName, char* clientName) {
    packet outPacket;
    int status;
    char serialized[PACKET_SIZE];

    setPacket(&outPacket,TYPE_UPLOAD_READY,0,0,0,fileName,clientName,"");

    serializePacket(&outPacket,serialized);
    status = write(sockfd,serialized,PACKET_SIZE);

    if (status < 0) {
        printf("ERROR writing to socket\n");
        return;
    }

}

void readyToListServer(int sockfd) {
    packet outPacket;
    int status;
    char serialized[PACKET_SIZE];

    setPacket(&outPacket,TYPE_LIST_SERVER_READY,0,0,0,"","","");
    serializePacket(&outPacket,serialized);
    status = write(sockfd,serialized,PACKET_SIZE);

    if (status < 0) {
        printf("ERROR writing to socket\n");
        return;
    }
}

int getSyncDirCommand(int sockfd, char* clientName) {
    packet outPacket;
    int status;
    char serialized[PACKET_SIZE];

    setPacket(&outPacket,TYPE_GET_SYNC_DIR,0,0,0,"",clientName,"");
    serializePacket(&outPacket,serialized);
    status = write(sockfd,serialized,PACKET_SIZE);

    if (status < 0) {
        printf("ERROR writing to socket\n");
        return ERRORCODE;
    }

    return SUCCESS;
}


void readyToSyncDir(int sockfd, char* clientName) {
    packet outPacket;
    int status;
    char serialized[PACKET_SIZE];

    setPacket(&outPacket,TYPE_GET_SYNC_DIR_READY,0,0,0,"",clientName,"");
    serializePacket(&outPacket,serialized);
    status = write(sockfd,serialized,PACKET_SIZE);

    if (status < 0) {
        printf("ERROR writing to socket\n");
        return;
    }
}


void uploadAll(int sockfd,char *pathToUser) {
    int status;
    char response[PAYLOAD_SIZE];
    char buffer[PACKET_SIZE] = {0};
    packet incomingPacket;
    DIR *dir;
    struct dirent *lsdir;

    bzero(response,PAYLOAD_SIZE);
    dir = opendir(pathToUser);
    /* print all the files within directory */
    while ((lsdir = readdir(dir)) != NULL )
    {
        if(strcmp(lsdir->d_name,".") !=0 && strcmp(lsdir->d_name,"..") !=0){
            sprintf(response,"%s",lsdir->d_name);
            status = write(sockfd, response, PAYLOAD_SIZE);
                if (status < 0) 
                printf("ERROR writing to socket\n");
            
            status = read(sockfd,buffer,PACKET_SIZE);
            if (status < 0) 
                printf("ERROR reading socket\n");
            deserializePacket(&incomingPacket,buffer);
            if (incomingPacket.type == TYPE_DOWNLOAD) {
                readyToUpload(sockfd,incomingPacket.fileName,incomingPacket.clientName);
                upload(sockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
            } else {
                printf("\nERROR Expected Download Packet\n");
                return;
            }
            
        }
    }
    closedir(dir);
    sprintf(response,"  ");
    status = write(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

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
    //char response[PAYLOAD_SIZE];
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


    //bzero(response, PAYLOAD_SIZE);
    /* read from the socket */
    /*
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s\n",response);
    */

    fclose(fp);
    
    //upload(sockfd, path, clientName, server);
    
    free(finalPath);
    
}

void inotifyDelCommand(int sockfd, char *path, char *clientName){
    
    char* fileName;
    char serialized[PACKET_SIZE];
    packet packetToDelete;
    int status;
    //char response[PAYLOAD_SIZE];

    fileName = getFileName(path);

    setPacket(&packetToDelete,TYPE_DELETE,0,0,0,fileName,clientName,"");

    serializePacket(&packetToDelete,serialized);

    /* write in the socket */

    status = write(sockfd, serialized, PACKET_SIZE);
    if (status < 0) 
        printf("ERROR writing to socket\n");

    
    /* captura o executing command */
    /*bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);*/
    /* captura a resposta da funcao delete */
    /*bzero(response, PAYLOAD_SIZE);
    status = read(sockfd, response, PAYLOAD_SIZE);
    if (status < 0) 
        printf("ERROR reading from socket\n");
    printf("%s", response);*/
}


