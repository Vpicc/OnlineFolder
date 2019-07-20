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
char synDir[9] = "_sync_dir";

void *handleConnection(void *socketDescriptor) {
    packet incomingPacket;
    char buffer[PACKET_SIZE];
    int exitCommand = FALSE;
    int n;
    int newsockfd = *(int*)socketDescriptor;
    int idUserName;
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
    strcpy(userName , buffer);
    /*strcat(userName, synDir);
    strcat(pathServerUsers, synDir);*/
    strcat(pathServerUsers,buffer);

    struct clientList *client_node = malloc(sizeof(*client_node));//node used to find the username on the list.

    if (!findNode(buffer, clientList, &client_node)){
        appendNewClient(newsockfd, buffer);
        checkAndCreateDir(pathServerUsers);
        sprintf(auth,"%s","authorized");
        write(newsockfd, auth, PACKET_SIZE);
    }
    else {
        if(!(updateNumberOfDevices(client_node, newsockfd, INSERTDEVICE) == SUCESS)){
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
                if(updateNumberOfDevices(client_node, newsockfd, REMOVEDEVICE)== SUCESS){
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
        
        switch(incomingPacket.type) {
            case TYPE_UPLOAD:
                readyToDownload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                download(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                uploadCommand(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                upload(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        uploadCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                        upload(otherSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
}
                break;
            case TYPE_INOTIFY:
                readyToDownload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                download(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        uploadCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                        upload(otherSocket,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                break;
            case TYPE_DOWNLOAD:
                readyToUpload(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                upload(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                break;
            case TYPE_INOTIFY_DELETE:
                delete(newsockfd,incomingPacket.fileName, pathServerUsers);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        deleteCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
                break;
            case TYPE_DELETE:
                delete(newsockfd,incomingPacket.fileName, pathServerUsers);
                deleteCommand(newsockfd,incomingPacket.fileName,incomingPacket.clientName);
                if(findNode(userName, clientList, &client_node)){
                    otherSocket = otherSocketDevice(incomingPacket.clientName, newsockfd);
                    if(otherSocket != -1){
                        deleteCommand(otherSocket,incomingPacket.fileName,incomingPacket.clientName);
                    }
                    else{
                        //nao tem outro device
                    }
                }
                else{
                    //cliente nem esta na lista
                }
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
                    if(updateNumberOfDevices(client_node, newsockfd, REMOVEDEVICE) == SUCESS){
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
    
    }

    printf("Closing Socket %d.\n", newsockfd);
	close(newsockfd);
    return 0;
}

void appendNewClient(int socketNewClient, char* userName) {
    struct client *newClient = malloc(sizeof(*newClient));
    newClient->devices[0] = socketNewClient;
    newClient->devices[1] = -1;
    strcpy(newClient->userName , userName);
    insertList(&clientList,*newClient);
}

int updateNumberOfDevices(struct clientList *client_node, int socketNumber, int option){
    if(option == INSERTDEVICE){
        if(client_node->client.devices[0] == FREEDEV)
        {
            client_node->client.devices[0] = socketNumber;
            return 1;
        }
        else if (client_node->client.devices[1] == FREEDEV)
        {
            client_node->client.devices[1] = socketNumber;
            return 1;
        }
        /*TODO: Avisar o client de que a conexão foi finalizada.
          "There is a connection limit of up to two connected devices.""
          Acho que vamos precisar fazer uma thread no client que fica só lendo resposta dos servidor,
          pq se não ele fica presso naquele printf "Enter command"..
        */
        else{ //all devices are busy
            return 0;
        }
    }
    //REMOVE DEVICE
    else{
        for(int device = 0; device <= 1; device++){
            if(client_node->client.devices[device] == socketNumber){
                client_node->client.devices[device] = FREEDEV;
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
