#include "../../include/linkedlist/linkedlist.h"

void createList(struct clientList *clientList)
{
	clientList = NULL;
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