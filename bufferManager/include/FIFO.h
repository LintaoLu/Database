#ifndef FIFO_H
#define FIFO_H
#include <stdio.h>
#include <stdlib.h>

/*Queue - Linked List implementation*/
typedef struct Node {
    int key;
    struct Page *page;
    struct Node *next;
} Node;

struct MapNode{
    int key;
    struct Node *val;
    struct MapNode *next;
};

struct table{
    int size;
    struct MapNode **list;
};

struct table *createFIFOTable(int size);
int hashCode(struct table *t, int key);
void FIFO_put(int x, char* data);
struct Page* FIFO_get(int key);
int FIFO_remove(struct table *t, int key);
void FIFO_Dequeue();
void insert(struct table *t, int key, struct Node* val);
void initFIFO(int capacity, int mapCapacity);
void FIFO_free();

#endif