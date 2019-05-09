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
#include "../../include/common/common.h"
#include "../../include/linkedlist/linkedlist.h"
#include "../../include/client/client.h"

void *listener(void *socket){
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

        printf("PACKET: %u %u %u %u %s %s %s\n", incomingPacket.type,incomingPacket.seqn,incomingPacket.length,incomingPacket.total_size,incomingPacket.clientName,incomingPacket.fileName,incomingPacket._payload);

        switch(incomingPacket.type) {
                case TYPE_UPLOAD:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    bzero(lastFile,100);
                    break;
                case TYPE_INOTIFY:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    bzero(lastFile,100);                    
                    break;
                case TYPE_DELETE:
                    delete(connectionSocket,incomingPacket.fileName, incomingPacket.clientName);
                    bzero(lastFile,100);                    
                    break;
                case TYPE_INOTIFY_DELETE:
                    delete(connectionSocket,incomingPacket.fileName, incomingPacket.clientName);
                    bzero(lastFile,100);                    
                    break;
                case TYPE_DOWNLOAD_READY:
                    upload(connectionSocket,clientPath,incomingPacket.clientName,FALSE);
                    break;
                case TYPE_UPLOAD_READY:
                    download(connectionSocket,incomingPacket.fileName,incomingPacket.clientName,FALSE);
                    break;
                case TYPE_LIST_SERVER_READY:
                    clientListServer(connectionSocket);
                    break;
                case TYPE_GET_SYNC_DIR_READY:
                    clientSyncServer(connectionSocket, incomingPacket.clientName);
                    printf("\nAll Files Updated.\n");
                    break;
                default:
                    break;
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
            } else {
                printf("\nERROR Expected Upload Ready Packet\n");
                return;
            }
        }

        
    } while (strcmp(response,"  ") != 0);
}
