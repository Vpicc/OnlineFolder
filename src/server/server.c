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

void *handleConnection(void *socketDescriptor) {
    packet incomingPacket;
    char buffer[PACKET_SIZE];
    int exitCommand = FALSE;
    int n;
    int newsockfd = *(int*)socketDescriptor;
    int idUserName;
    char *userName = malloc(sizeof(userName));
    char pathServerUsers[30] = "";


  
    //TODO: get_sync_dir, creates directory, if not created


    //TODO: Thread to receive updates from client

    /*************************************/
    //Reads the client name and update/search on the client list.
    bzero(buffer, PAYLOAD_SIZE);
    idUserName = read(newsockfd,buffer,PAYLOAD_SIZE);
    if(idUserName < 0)
        printf("ERROR reading from socket");
    strcpy(userName , buffer);
    strcat(pathServerUsers,buffer);

    struct clientList *client_node = malloc(sizeof(*client_node));//node used to find the username on the list.

    if(!findNode(buffer, clientList, &client_node)){
        appendNewClient(newsockfd, buffer);
        /*
        Cria se necessario a pasta do usuario no servidor
        */
        checkAndCreateDir(pathServerUsers);
        /*
        Lança a thread que fica olhando o diretorio do servidor
        if(pthread_create(&thread_id, NULL, inotifyWatcher, (void *) pathServerUsers) < 0){
			    fprintf(stderr,"ERROR, could not create thread.\n");
			    exit(-1);
		}*/
        write(newsockfd, "authorized", 11);
    }
    else{
        if(!(updateNumberOfDevices(client_node, newsockfd, INSERTDEVICE) == SUCESS)){
            exitCommand = TRUE;
            printf("There is a connection limit of up to two connected devices.Will close this client device.\n");
            write(newsockfd, "notauthorized", 14);
        }
        else{
            /*
            Cria se necessario a pasta do usuario no servidor
            */
            checkAndCreateDir(pathServerUsers);
            /*
            Lança a thread que fica olhando o diretorio do servidor
            
            if(pthread_create(&thread_id, NULL, inotifyWatcher, (void *) pathServerUsers) < 0){
			    fprintf(stderr,"ERROR, could not create thread.\n");
			    exit(-1);
		    }
            */
            write(newsockfd, "authorized", 11);
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
        n = write(newsockfd,"Executing Command...", 22);
        if (n < 0) 
            printf("ERROR writing to socket");

        
        deserializePacket(&incomingPacket,buffer);
        
        switch(incomingPacket.type) {
            case TYPE_UPLOAD:
                download(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                break;
            case TYPE_DOWNLOAD:
                upload(newsockfd,incomingPacket.fileName,incomingPacket.clientName,TRUE);
                break;
            case TYPE_DELETE:
            // delete from syncd dir
                break;
            case TYPE_LIST_SERVER:
            // list user's saved files on dir
                break;
            case TYPE_LIST_CLIENT:
            // list saved files on dir
                break;
            case TYPE_GET_SYNC_DIR:
            // creates sync_dir_<username> and syncs
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

