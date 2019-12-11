#ifndef LRU_H
#define LRU_H
// A C program to show implementation of LRU cache
#include <stdio.h> 
#include <stdlib.h> 

// A Queue Node (Queue is implemented using Doubly Linked List) 
typedef struct QNode {
	struct QNode *prev, *next;
	int pageNumber; // the page number stored in this QNode 
	struct Page* page;
} QNode;

// A Queue (A FIFO collection of Queue Nodes) 
typedef struct Queue {
	int count; // Number of filled frames 
	int numberOfFrames; // total number of frames 
	QNode *front, *rear;
} Queue;

// A hash (Collection of pointers to Queue Nodes) 
typedef struct Hash {
	int capacity; // how many pages can be there 
	QNode** array; // an array of queue nodes 
} Hash;

void initLRU(int queueSize, int hashSize);
struct Page* LRU_get(int pageNumber);
void LRU_put(int pageNumber, char* data);
void LRU_Enqueue(Queue* queue, Hash* hash, int pageNumber, char* data);
void LRU_deQueue(Queue* queue, QNode *curr);
int isQueueEmpty(Queue* queue);
int AreAllFramesFull(Queue* queue);
Hash* createHash(int capacity);
Queue* createQueue(int numberOfFrames);
QNode* newQNode(int pageNumber, char* data);
struct Page* findLRUPage(int pageNumber);
void LRU_free();

#endif