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

pthread_mutex_t clientInitMutex = PTHREAD_MUTEX_INITIALIZER;

void *handleConnection(void *socketDescriptor) {
    packet incomingPacket;
    char buffer[PACKET_SIZE] = {0};
    char clientIp[PACKET_SIZE] = {0};
    int exitCommand = FALSE;
    int n;
    int newsockfd = *(int*)socketDescriptor;
    int idUserName;
    int idClientIp;
    char *userName = malloc(sizeof(userName));
    char pathServerUsers[CLIENT_NAME_SIZE] = "";
    char auth[PACKET_SIZE] = {0};
    int otherSocket;




    /*************************************/
    //Reads the client name and update/search on the client list.
    bzero(buffer, PAYLOAD_SIZE);
    idUserName = read(newsockfd,buffer,PAYLOAD_SIZE);
    if(idUserName < 0)
        printf("ERROR reading from socket");

    idClientIp = read(newsockfd,clientIp,PAYLOAD_SIZE);
    if(idClientIp < 0)
        printf("ERROR reading IP from socket");

    strcpy(userName , buffer);
    strcat(pathServerUsers,buffer);

    struct clientList *client_node = malloc(sizeof(*client_node));//node used to find the username on the list.
    pthread_mutex_lock(&clientInitMutex);
    if (!findNode(buffer, clientList, &client_node)){
        appendNewClient(newsockfd, buffer, clientIp);
        checkAndCreateDir(pathServerUsers);
        sprintf(auth,"%s","authorized");
        write(newsockfd, auth, PACKET_SIZE);
    }
    else {
        if(!(updateNumberOfDevices(client_node, newsockfd, INSERTDEVICE, clientIp) == SUCESS)){
            exitCommand = TRUE;
            printf("There is a connection limit of up to two connected devices.Will close this client device.\n");
            sprintf(auth,"%s","notauthorized");
            write(newsockfd, auth, PACKET_SIZE);
        }
        else{
            checkAndCreateDir(pathServerUsers);
            sprintf(auth,"%s","authorized");
            write(newsockfd, auth, PACKET_SIZE);
        }
    }
    pthread_mutex_unlock(&clientInitMutex);

    /********************************/
    // MANDA IP DO NOVO CLIENTE PROS SERVIDORES SECUNDARIOS

    /********************************************/
	while(exitCommand == FALSE) {
        bzero(buffer, PACKET_SIZE);
        /* read from the socket */
        n = read(newsockfd, buffer, PACKET_SIZE);
        if (n < 0) 
            printf("ERROR reading from socket");


        if(n == 0) {
            printf("Empty response, closing\n");
            if(findNode(userName, clientList, &client_node)){
                if(updateNumberOfDevices(client_node, newsockfd, REMOVEDEVICE, "whatever")== SUCESS){
                    exitCommand = TRUE;
                }
                else{
                    printf("ERROR updating the client list");
                }
            }
            else{
                printf("ERROR searching the client");
            }
        }

        /* write in the socket */
        /*n = write(newsockfd,"Executing Command...\n", 23);
        if (n < 0) 
            printf("ERROR writing to socket");
        */
        
        deserializePacket(&incomingPacket,buffer);

        if(findNode(userName, clientList, &client_node)){
            pthread_mutex_lock(&(client_node->client.clientPairMutex));
        }
        
        switch(incomingPacket.type) {
            case TYPE_UPLOAD:
                readyToDownload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                download(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                uploadCommand(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                upload(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        mirrorUploadCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                // mandar pra todos os servers
                // for de servers -> mirrorUploadCommand
                break;
            case TYPE_INOTIFY:
                readyToDownload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                download(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                inotifyConfirmation(newsockfd,incomingPacket.fileName,pathServerUsers);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        mirrorUploadCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                // mandar pra todos os servers
                // for de servers -> mirrorUploadCommand
                break;
            case TYPE_DOWNLOAD:
                readyToUpload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                upload(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                break;
            case TYPE_INOTIFY_DELETE:
                delete(newsockfd,incomingPacket.fileName, pathServerUsers);
                inotifyConfirmation(newsockfd,incomingPacket.fileName,pathServerUsers);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        inotifyDelCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                // mandar pra todos os servers
                // for de servers -> inotifyDelCommand
                break;
            case TYPE_DELETE:
                delete(newsockfd,incomingPacket.fileName, pathServerUsers);
                inotifyDelCommand(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        inotifyDelCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                // for de servers -> inotifyDelCommand
                break;
            case TYPE_LIST_SERVER:
                readyToListServer(newsockfd);
                list_files(newsockfd, pathServerUsers, TRUE);
                break;
            case TYPE_GET_SYNC_DIR:
                readyToSyncDir(newsockfd,incomingPacket.clientName);
                uploadAll(newsockfd,pathServerUsers);
                break;
            case TYPE_EXIT:
                printf("Exit command, closing connection\n");
                if(findNode(userName, clientList, &client_node)){
                    if(updateNumberOfDevices(client_node, newsockfd, REMOVEDEVICE, "whatever") == SUCESS){
                        exitCommand = TRUE;
                    }
                    else{
                        printf("ERROR updating the client list");
                    }
                }
                else{
                    printf("ERROR searching the client");
                }
                break;
            default:
                break;
        }
        if(findNode(userName, clientList, &client_node)){
            pthread_mutex_unlock(&(client_node->client.clientPairMutex));
        }
    
    }

    printf("Closing Socket %d.\n", newsockfd);
	close(newsockfd);
    return 0;
}

void appendNewClient(int socketNewClient, char* userName, char* clientIp) {
    struct client *newClient = malloc(sizeof(*newClient));
    newClient->devices[0] = socketNewClient;
    newClient->devices[1] = -1;
    strcpy(newClient->userName , userName);
    strcpy(newClient->ip[0], clientIp);
    bzero(newClient->ip[1], 100);
    pthread_mutex_init(&(newClient->clientPairMutex), NULL);
    insertList(&clientList,*newClient);
}

int updateNumberOfDevices(struct clientList *client_node, int socketNumber, int option, char* clientIp){
    if(option == INSERTDEVICE){
        if(client_node->client.devices[0] == FREEDEV)
        {
            strcpy(client_node->client.ip[0], clientIp);
            client_node->client.devices[0] = socketNumber;
            return 1;
        }
        else if (client_node->client.devices[1] == FREEDEV)
        {
            client_node->client.devices[1] = socketNumber;
            strcpy(client_node->client.ip[1], clientIp);
            return 1;
        }
        else{ //all devices are busy
            return 0;
        }
    }
    //REMOVE DEVICE
    else{
        for(int device = 0; device <= 1; device++){
            if(client_node->client.devices[device] == socketNumber){
                client_node->client.devices[device] = FREEDEV;
                bzero(client_node->client.ip[device], 100);
            }
        }
        return 1;
    }

    return 0;
}


int otherSocketDevice (char *userName, int actSocket) {
    int otherSocketDevice;
    struct clientList *client_node = malloc(sizeof(*client_node));//node used to find the username on the list.
    if(findNode(userName, clientList, &client_node)){
        if(client_node->client.devices[0] == actSocket){
            if(client_node->client.devices[1] != -1){
                //tem outro device
                otherSocketDevice = client_node->client.devices[1];
                return otherSocketDevice;
            }
            else{
                return -1;
            }
        }
        else{
            if(client_node->client.devices[0] != -1){
                //tem outro device
                otherSocketDevice = client_node->client.devices[0];
                return otherSocketDevice;
            }
            else{
                return -1;
            }
        }
    }
    else{
        return -1;
    }

}

void *createServerPrimary(){

    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    //pthread_t thread_id;

    printf("Opening Socket...\n");
    //socket para conexão de replicas
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr,"ERROR opening socket.\n");
		exit(-1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVERPORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    

	printf("Binding Socket... -- PRIMARY SERVER\n");

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr,"ERROR on binding.\n");
		exit(-1);
	}
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);

	printf("Accepting new connections... -- PRIMARY SERVER\n");

	while ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) != -1) {
		printf("Connection Accepted -- PRIMARY SERVER\n");

        //get_sync_dir versao server
        // read do request do cliente
        // dai começa o get sync dir server


		printf("Handler Assigned -- PRIMARY SERVER\n");

	}
		
	close(sockfd);
	return 0; 
}



void copyIp(char *token,char *ipToken) {
    int i = 0;
    while((char) *(token + i) != ';') {
        ipToken[i] = (char) *(token + i);
        i++;
    }
    ipToken[i] = '\0';
}