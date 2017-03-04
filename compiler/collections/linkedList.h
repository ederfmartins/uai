#ifndef LISTA_H_ 
#define LISTA_H_ 

typedef struct NodeList_str
{
	void* value;
	struct NodeList_str* Prox;
} NodeList;

typedef struct
{
	struct NodeList_str* head;
	struct NodeList_str* tail;
	int cnt;
	void (*recursiveFree)(void* arg);
} LinkedList;

/*
 * Construtor da classe lista encadeada.
 * Parametro: list -> a lista a ser inicialisada
 */
void ll_init(LinkedList* list);
/*
 * Cria uma copia de src em dest. Caso dest tenha elementos
 * eles seram apagados antes que src seja clonado em dest
 * Parametro: src -> Lista encadeada que sera clonada.
 * Parametro: dest -> Lista que recebera uma copia de src.
 * dest ja deve ter sido inicializada. Observe que apenas o
 * valor do ponteiro value eh copiado, e nao seu conteudo.
 */
void ll_clone(LinkedList* src, LinkedList* dest);
/*
 * Esvazia a lista, mas nao a destroi.
 * Parametro: list -> implicit this
 */
void ll_clear(LinkedList* list);
/*
 * destrutor da classe lista encadeada.
 * Parametro: list -> a lista a ser destruida
 */
void ll_destroy(LinkedList* list);
/*
 * Parametro: list -> implicit this
 * Retorna: true se a lista esta vazia, false em caso contrario.
 */
int ll_isEmpyt(LinkedList* list);
/*
 * Insere o elemento value no fim da lista.
 * Parametro: list -> implicit this
 * Parametro: value -> ponteiro para o elemento a ser inserido.
 * Observe que a lista nao se responsabilisa se value eh ou nao
 * um apontador para uma area de memoria valida.
 * Retorna: true se o elemento foi inserido sem problemas, false se nao foi
 * possivel inserir o elemento.
 */
int ll_insert(LinkedList *list, void* value);

/*
 *
 */
void ll_transfer(LinkedList *dest, LinkedList *orig);
/*
 * Parametro: list -> implicit this
 * Retorna: o tamanho da lista
 */
#define ll_size(list) (list)->cnt
/*
 * Informa a lista que ela eh responsavel por limpar a memoria
 * alocada em value. Esse parametro nao deve ser usado caso a 
 * estrutura precise de uma limpeza elaborada de memoria.
 * Parametro: list -> implicit this
 */
#define ll_setRecursiveFree(list, func) (list)->recursiveFree = (func)
/*
 * Parametro: apontadorlista -> implicit this
 * Retorna: o valor do elemento apontado por this
 */
#define nl_getValue(apontadorlista) (apontadorlista)->value  

/*
 * Parametro: list -> implicit this
 * Retorna: o elemento do topo da lista.
 */
void* ll_getFirstElement(LinkedList* list);
/*
 * Parametro: list -> implicit this
 * Retorna: o nodo do topo da lista.
 */
NodeList* ll_getFirstNode(LinkedList* list);
/*
 * Parametro: list -> implicit this
 * Retorna o elemento do fundo da lista.
 */
void* ll_getLastElement(LinkedList* list);
/*
 * Parametro: list -> implicit this
 * Remove o elemento do topo da lista.
 */
void* ll_removeFirstElement(LinkedList* list);

NodeList* ll_iter_begin(LinkedList* list);
NodeList* ll_iter_end(LinkedList* list);
NodeList* ll_iter_next(NodeList* list);

#endif
