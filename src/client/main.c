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


int main(int argc, char *argv[])
{
    int sockfd;
    int authorization = WAITING;
    int exitCommand = FALSE;
    char command[PAYLOAD_SIZE];
    char response[PAYLOAD_SIZE];
    char *option;
    char *path;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int idUserName;
    pthread_t thread_id;

    bzero(command,PAYLOAD_SIZE);

    if (argc < 3) {
		fprintf(stderr,"usage %s hostname\n", argv[0]);
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


    //TODO: get_sync_dir, creates directory, if not created
    
    idUserName = write(sockfd, argv[1], strlen(argv[1]));
    // envia o username para o servidor
    if (idUserName < 0) 
        printf("ERROR writing to socket\n");
    /*
    Espera autorização do servidor para validar a conexão
    */
    while(authorization == WAITING){
        read(sockfd, response, PAYLOAD_SIZE);
        if(strcmp(response,"authorized") == 0){
            checkAndCreateDir(argv[1]);
            if(pthread_create(&thread_id, NULL, inotifyWatcher, (void *) argv[1]) < 0){
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

    //TODO: Thread to receive updates from server

    while (exitCommand == FALSE) {

        printf("\nEnter the Command: ");
        bzero(command, PAYLOAD_SIZE);
        fgets(command, PAYLOAD_SIZE, stdin);

        option = strtok(command," ");
        path = strtok(NULL," ");
        if(path != NULL) {
            path = strtok(path,"\n");
        }

        printf("OPTION: %s\n", option);
        printf("PATH: %s\n", path);
        

        // Switch for options
        if(strcmp(option,"exit\n") == 0) {
            exitCommand = TRUE;
        } else if (strcmp(option, "upload") == 0) { // upload from path
            uploadCommand(sockfd,path,argv[1], FALSE);          
        } else if (strcmp(option, "download") == 0) { // download to exec folder
            downloadCommand(sockfd,path,argv[1], FALSE);
        } else if (strcmp(option, "delete") == 0) { // delete from syncd dir
            
        } else if (strcmp(option, "list_server") == 0) { // list user's saved files on dir
            
        } else if (strcmp(option, "list_client") == 0) { // list saved files on dir
            
        } else if (strcmp(option, "get_sync_dir") == 0) { // creates sync_dir_<username> and syncs
            
        } else if (strcmp(option, "printar") == 0) { // creates sync_dir_<username> and syncs
        }

    }

	close(sockfd);

    printf("Connection Closed. Exiting Program.\n");
    return 0;
}