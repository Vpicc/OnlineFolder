#include "../../include/linkedlist/linkedlist.h"
int id = 0;

void createList(struct clientList *clientList)
{
	clientList = NULL;
}
int isPrimary(char *serverName, int port, struct serverList **serverList){

	struct serverList *pointer = *serverList;
	struct serverList *anotherPointer = *serverList;

	do{
		if(strcmp(serverName, pointer->serverName) == 0 && pointer->port == port)
			return pointer->isPrimary;
		else
		{
			pointer = pointer->next;
		}
	}while(pointer != anotherPointer);
	return 0;
}

int setPrimary(int serverID, struct serverList **serverList){

	struct serverList *pointer = *serverList;
	struct serverList *anotherPointer = *serverList;

	do{
		if(pointer->id == serverID){
			pointer->isPrimary=1;
			return 1;
		}else
		{
			pointer = pointer->next;
		}
	}while(pointer != anotherPointer);
	return 0;
}

struct serverList *primaryServer(struct serverList **serverList){
	
	struct serverList *pointer = *serverList;
	struct serverList *anotherPointer = *serverList;

	do{
		if(pointer->isPrimary == TRUE)
			return pointer;
		else
		{
			pointer = pointer->next;
		}
	}while(pointer != anotherPointer);

	return NULL;
}
struct serverList *previousServer(char *serverName, int myPORT, struct serverList **serverList){
	struct serverList *pointer = *serverList;

	while(pointer != NULL){
		if(strcmp(serverName, pointer->serverName) == 0 && pointer->port == myPORT)
			return pointer->previous;
		else
		{
			pointer = pointer->next;
		}
	}
	return NULL;
}

struct serverList *findServer(char *serverName, int port, struct serverList **serverList){
	struct serverList *pointer = *serverList;

	while(pointer != NULL){
		if(strcmp(serverName, pointer->serverName) == 0 && pointer->port == port)
			return pointer;
		else
		{
			pointer = pointer->next;
		}
	}
	return NULL;
}
void createServerList(struct serverList *serverList)
{
	serverList = NULL;
}
void insertServerList(struct serverList **serverList, char *name, int port){

	struct serverList *server_node= malloc(sizeof(struct serverList));
	struct serverList *pointer = *serverList;
	struct serverList *beginPointer = *serverList;

	//struct serverList *anotherPointer = *serverList;
	server_node->next=server_node;
	server_node->previous=server_node;
	/* if(strcspn(name, "\n")>0)
        name[strcspn(name, "\n")] = 0;*/
	strcpy(server_node->serverName,name);
	server_node->port = port;
	server_node->id = id++;
	server_node->isPrimary=FALSE;

	if(*serverList == NULL)
	{
		server_node->isPrimary=TRUE;
		*serverList = server_node;
		return;
	}
	else
	{
		while(pointer->next != beginPointer)
			pointer= pointer->next;

		pointer->next = server_node;
		pointer->next->previous=pointer;
		pointer->next->next=beginPointer;
		beginPointer->previous=pointer->next;
	}
	return;
}
int insertList(struct clientList **clientList, struct client client)
{
	struct clientList *client_node;
	struct clientList *clientList_aux = *clientList;

	client_node = malloc(sizeof(struct clientList));

	client_node->client = client;
	client_node->next = NULL;

	if (*clientList == NULL)
	{
		*clientList = client_node;
		return 1;
	}
	else
	{
		while(clientList_aux->next != NULL)
			clientList_aux = clientList_aux->next;

		clientList_aux->next = client_node;
	}
	return 0;
}

int isEmpty(struct clientList *clientList)
{
	return clientList == NULL;
}

int findNode(char *userid, struct clientList *clientList, struct clientList **client_node)
{
	struct clientList *clientList_aux = clientList;

	while(clientList_aux != NULL)
	{
		if (strcmp(userid, clientList_aux->client.userName) == 0)
		{
			*client_node = clientList_aux;
			return 1;
		}
		else
			clientList_aux = clientList_aux->next;
	}
	return 0;
}

int removeFromServerList(struct serverList **serverList, char* primaryServerIp, int primaryServerPort) {
	struct serverList *pointer = *serverList;
	struct serverList *beginPointer = *serverList;

	do{
		if(strcmp(primaryServerIp, pointer->serverName) == 0 && pointer->port == primaryServerPort){
			if(pointer == *serverList) {
				if(pointer->next != NULL)
					*serverList = pointer->next;
			}
			pointer->previous->next = pointer->next;
			pointer->next->previous = pointer->previous;
			free(pointer);
			return TRUE;
		} else {
			pointer = pointer->next;
		}
	}while(pointer != beginPointer);
	return FALSE;
}


