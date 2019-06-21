#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include "../../include/common/common.h"
#include "../../include/linkedlist/linkedlist.h"
#include "../../include/client/client.h"
#include "../../include/server/secondary.h"

sem_t listenRingSemaphore;
sem_t writeRingSemaphore;

int highestID = -1;
int participant = FALSE;
int FINISHED = FALSE;
int hasElected = FALSE;
int primaryServerSockfd;

void secondaryServer(char *primaryServerIp,int primaryServerPort){
//TODO: CONECTAR AO SERVIDOR E REPLICAR TUDO O QUE HÁ NELE - PASTAS/ARQUIVOS
   // int initialization = 1;
    //pthread_t thread_inotify, thread_listener, thread_writer, thread_reconnection;
    //struct inotyClient *inotyClient = malloc(sizeof(*inotyClient));
    int primaryServerSockfd;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    char buffer[PAYLOAD_SIZE];
    packet incomingPacket;
    int status;
    struct serverList* myServerNode;
    myServerNode = findServer(ip,myPORT,&serverList);
    
    server = gethostbyname(primaryServerIp);

	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    
    if ((primaryServerSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(SERVERPORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
	bzero(&(serv_addr.sin_zero), 8);

    
	if (connect(primaryServerSockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(-1);
    }

    while(1){
        status = read(primaryServerSockfd,buffer,PAYLOAD_SIZE);
        if(status == 0){
            if(myServerNode->next != myServerNode)
                removeFromServerList(&serverList,primaryServerIp,primaryServerPort);
            close(primaryServerSockfd);
            election();
            setPrimary(highestID, &serverList);
            return;
        }

        deserializePacket(&incomingPacket,buffer);

        switch(incomingPacket.type) {
            case TYPE_MIRROR_UPLOAD:
                strcpy(lastFile,incomingPacket.fileName);
                downloadCommand(serverSockfd,incomingPacket.fileName,incomingPacket.clientName,FALSE);
                read(serverSockfd, buffer, PACKET_SIZE);
                deserializePacket(&incomingPacket,buffer);
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
            /*
            case get_sync_dir_server:
                write pro server pedindo os arquivos
                // entra numa função que fica mandando download request: olhar clientSyncServer no client.c
             */
            /*
            case NEW_CLIENT:
                // coloca na lista de clientes
             */
            default:
                break;
        }
                
    }


}
void election() {
    sem_init(&listenRingSemaphore,0,0);
    sem_init(&writeRingSemaphore,0,1);
    highestID = -1;
    participant = FALSE;
    FINISHED = FALSE;
    hasElected = FALSE;
    createServerRing();
    sem_close(&listenRingSemaphore);
    sem_close(&writeRingSemaphore);

}

void createServerRing(){

    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t thread_id, thread_id2;

    printf("Opening Socket... RING\n");
    //socket conexao anel
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr,"ERROR opening socket.\n");
		exit(-1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ELECTIONPORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    

	printf("Binding Socket... -- RING\n");

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr,"ERROR on binding.\n");
		exit(-1);
	}
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);

	printf("Accepting new connections... -- RING\n");

    if(pthread_create(&thread_id, NULL, writeToRing, NULL) < 0){
		fprintf(stderr,"ERROR, could not create thread.\n");
		exit(-1);
	}
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) != -1) {
		printf("Connection Accepted -- RING\n");

		if(pthread_create(&thread_id2, NULL, listenFromRing, (void*)&newsockfd) < 0){
			fprintf(stderr,"ERROR, could not create thread.\n");
			exit(-1);
		}

		printf("Handler Assigned\n");

	}
    while(!FINISHED){};

	close(sockfd);
	return; 
}




void *listenFromRing(void *socketDescriptor) {
    int sockfd = *(int*)socketDescriptor;
    int status;
    char buffer[PACKET_SIZE];
    char* electionStatus;
    char* receivedElectionId;
    struct serverList* myServerNode;
    myServerNode = findServer(ip,myPORT,&serverList);

    while(!FINISHED) {
        sem_wait(&listenRingSemaphore);
        if(!FINISHED){
            status = read(sockfd,buffer,PACKET_SIZE);

            if(status < 0) {
                printf("ERROR reading from socket -- Ring");
                exit(-1);
            }

            electionStatus = strtok(buffer,",");

            if(strcmp("ELECTION",electionStatus) == 0) {
                electionStatus = strtok(NULL,",");
                receivedElectionId = electionStatus;
                if(atoi(receivedElectionId) > myServerNode->id) {
                    highestID = atoi(receivedElectionId);
                } else {
                    highestID = myServerNode->id;
                }
            } else if(strcmp("ELECTED",electionStatus) == 0) {
                electionStatus = strtok(NULL,",");
                receivedElectionId = electionStatus;
                highestID = atoi(receivedElectionId);
                // mudar servidor primario
                FINISHED = TRUE;
                hasElected = TRUE;
            }
            electionStatus = NULL;
            receivedElectionId = NULL;
        }
        sem_post(&writeRingSemaphore);
    }
    
    return NULL;
}


void *writeToRing() {
    int serverSockfd;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    char buffer[PACKET_SIZE];
    struct serverList* previousServerNode;
    struct serverList* myServerNode;
    previousServerNode = previousServer(ip,myPORT,&serverList);
    myServerNode = findServer(ip,myPORT,&serverList);
    server = gethostbyname(previousServerNode->serverName);

	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }
    
    if ((serverSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket\n");
        exit(-1);
    }
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(ELECTIONPORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
	bzero(&(serv_addr.sin_zero), 8);

    
	while (connect(serverSockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0);

    printf("RING STARTED\n");

    while(!FINISHED) {
        sem_wait(&writeRingSemaphore);
        if(!hasElected){
            if(highestID < myServerNode->id && !participant){
                participant = TRUE;
                sprintf(buffer,"ELECTION,%d",myServerNode->id);

                write(serverSockfd,buffer,PACKET_SIZE);
            } else if (highestID > myServerNode->id){
                sprintf(buffer,"ELECTION,%d",highestID);

                write(serverSockfd,buffer,PACKET_SIZE);
            } else if(highestID == myServerNode->id && participant){
                sprintf(buffer,"ELECTED,%d",myServerNode->id);

                write(serverSockfd,buffer,PACKET_SIZE);
                //definir como primario
                FINISHED = TRUE;
                hasElected = TRUE;
            }
        } else {
            sprintf(buffer,"ELECTED,%d",highestID);
            write(serverSockfd,buffer,PACKET_SIZE);
        }

        sem_post(&listenRingSemaphore);
    }

    return NULL;
}
