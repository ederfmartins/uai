#include <stdio.h>
#include <stdlib.h>

#include "linkedList.h"

void ll_init(LinkedList* list)
{
	list->head = (NodeList*) malloc(sizeof(NodeList));
	list->head->Prox = list->head;
	list->head->value = NULL;
	list->tail = list->head;
	list->cnt = 0;
	list->recursiveFree = NULL;
}
void ll_clear(LinkedList* list)
{
	/*if (list->cnt > 0)
	{
		NodeList* aux = list->head->Prox;
		NodeList* aux1 = NULL;

		while (aux != list->head)
		{
			aux1 = aux->Prox;
			if (list->recursiveFree != NULL)
				(list->recursiveFree)(aux->value);
			free(aux);
			aux = aux1;
		}
		list->head->Prox = list->head;
		list->tail = list->head;
		list->cnt = 0;
	}*/
	ll_destroy(list);
	ll_init(list);
}
void ll_clone(LinkedList* src, LinkedList* dest)
{
	int i;
	NodeList* node = NULL;

	ll_clear(dest);
	
	if(src->cnt > 0)
	{
		node = src->head->Prox;
				
		for (i = 0; i < ll_size(src); i++)
		{
			ll_insert(dest, node->value);
			node = node->Prox;
		}
		dest->cnt = src->cnt;
	}
}
void ll_destroy(LinkedList* list)
{
	//if (list->cnt > 0)
	//{
		//NodeList* aux = list->head->Prox;
		NodeList* aux = list->head;
		NodeList* aux1 = NULL;
		while (aux != list->tail)
		{
			aux1 = aux->Prox;
			if (list->recursiveFree != NULL)
				(list->recursiveFree)(aux->value);
			free(aux);
			aux = aux1;
		}
		if (list->recursiveFree != NULL)
			(list->recursiveFree)(aux->value);
		free(aux);
	//}
	//else if (list->cnt == 0 && list->head != NULL)
	//{
	//	free(list->head);
	//}
	list->head = NULL;
	list->tail = NULL;
	list->cnt = 0;
	list->recursiveFree = 0;
}

int ll_isEmpy(LinkedList* list)
{
	return (list->cnt == 0);
}

int ll_insert(LinkedList *list, void* value)
{
	NodeList* temp = (NodeList*) malloc(sizeof(NodeList));

	if (temp == NULL)
	{
		printf("Ocorreu um erro em Insere de lista.c, memoria insuficiente\n");
		return 0;
	}

	temp->value = value;
	temp->Prox = list->head;
	list->tail->Prox = temp;
	list->tail = temp;

	list->cnt++;
	return 1;
}

void ll_transfer(LinkedList *dest, LinkedList *orig)
{
	if (ll_size(orig) > 0) {
		dest->tail->Prox = orig->head->Prox;
		dest->tail = orig->tail;
		dest->tail->Prox = dest->head;
		dest->cnt += orig->cnt;

		orig->head->Prox = orig->head;
		orig->tail = orig->head;
		orig->cnt = 0;
	}
}

void* ll_getFirstElement(LinkedList* list)
{
	if (list-> cnt > 0)
		return list->head->Prox->value;
	return NULL;
}

NodeList* ll_getFirstNode(LinkedList* list)
{
	if (list->cnt > 0)
		return list->head->Prox;
	return NULL;
}

void* ll_getLastElement(LinkedList* list)
{
	if (list->cnt > 0)
		return list->tail->value;
	return NULL;
}

void* ll_removeFirstElement(LinkedList* list)
{
	if (list->cnt > 0)
	{
		NodeList* aux = list->head->Prox;
		void* value = aux->value;

		list->head->Prox = aux->Prox;

		if (aux == list->tail)
			list->tail = list->head;

		list->cnt--;
		free(aux);
		return value;
	}
	return NULL;
}

NodeList* ll_iter_begin(LinkedList* list) {
	return list->head->Prox;
}

NodeList* ll_iter_end(LinkedList* list) {
	return list->head;
}

NodeList* ll_iter_next(NodeList* node) {
	return node->Prox;
}
