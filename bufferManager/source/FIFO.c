#include "FIFO.h"
#include "Page.h"

// Two glboal variables to store address of front and rear nodes. 
struct Node* front = NULL;
struct Node* rear = NULL;
int size = 0, currSize = 0;
struct table *t;
extern int pinned_page;
extern Page* evictedPage;

struct table *createFIFOTable(int size){
	struct table *t = (struct table*)malloc(sizeof(struct table));
	t->size = size;
	t->list = (struct MapNode**)malloc(sizeof(struct MapNode*)*size);
	int i;
	for (i = 0; i<size; i++)
		t->list[i] = NULL;
	return t;
}

int hashCode(struct table *t, int key){
	if (key<0)
		return -(key%t->size);
	return key%t->size;
}


//insert hashmap
void insert(struct table *t, int key, struct Node* val){
	int pos = hashCode(t, key);
	struct MapNode *list = t->list[pos];
	struct MapNode *newNode = (struct MapNode*)malloc(sizeof(struct MapNode));
	struct MapNode *temp = list;
	while (temp){
		if (temp->key == key){
			temp->val = val;
			return;
		}
		temp = temp->next;
	}
	newNode->key = key;
	newNode->val = val;
	newNode->next = list;
	t->list[pos] = newNode;
}

struct Page* FIFO_get(int key){
	int pos = hashCode(t, key);
	struct MapNode *list = t->list[pos];
	struct MapNode *temp = list;
	while (temp){
		if (temp->key == key){
			return temp->val->page;
		}
		temp = temp->next;
	}
	return NULL;
}

//remove item form hashmap
int FIFO_remove(struct table *t, int key){
	int pos = hashCode(t, key);
	if (t->list[pos]){
		t->list[pos] = NULL;
		return 1;
	}
	return -1;
}

void initFIFO(int capacity, int mapCapacity){
	size = capacity;
	t = createFIFOTable(mapCapacity);
}

void FIFO_put(int x, char* data) {
    if (size == 0) return;
    if (pinned_page == size) {
        evictedPage = malloc(1 * sizeof(Page));
        evictedPage->pageValue = NULL;
        evictedPage->pageNumber = -1;
        return;
    }
	if (currSize == size){
		FIFO_Dequeue();
	}
	currSize++;
	struct Node* temp =
		(struct Node*)malloc(sizeof(struct Node));
	struct Page* page = (struct Page*)malloc(sizeof(struct Page));
	page->isDirty = 0;
	page->isPinned = 0;
	page->pageValue = data;
	page->pageNumber = x;
	page->fixCount = 0;
	temp->page = page;
	temp->key = x;
	temp->next = NULL;
	insert(t, x, temp);
	if (front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

void FIFO_Dequeue() {
	struct Node* curr = front, *pre = NULL;
	while (curr != NULL && curr->page->isPinned != 0){
		pre = curr;
		curr = curr->next;
	}
	if (curr == NULL) return;
	FIFO_remove(t, curr->key);
	
	if (front == rear && curr == front) {
		front = rear = NULL;
	}
	else if (curr == front) front = front->next;
	else if (curr == rear) rear = pre;
	else pre->next = curr->next;
	if(curr->page->isDirty) evictedPage = curr->page;
	else{
            free(curr->page);
            evictedPage = NULL;
	    }

	currSize--;
	free(curr);
}

void FIFO_free(){
	while (front != NULL){
		Node *temp = front;
		front = front->next;
		if (temp->page != NULL) free(temp->page);
		free(temp);
	}
	for(int i = 0; i < t->size; i++){
	    if(t->list[i] != NULL) free(t->list[i]);
	}
	t = NULL;
    front = rear = NULL;
    size = 0; currSize = 0;
}
