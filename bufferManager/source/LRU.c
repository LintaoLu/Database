#include "LRU.h"
#include "Page.h"

Queue* queue;
Hash* hash;
extern Page* evictedPage;
extern int pinned_page;

void initLRU(int queueSize, int hashSize){
	queue = createQueue(queueSize);
	hash = createHash(hashSize);
}

// This function is called when a page with given 'pageNumber' is referenced 
// from cache (or memory). There are two cases: 
// 1. Frame is not there in memory, we bring it in memory and add to the front 
// of queue 
// 2. Frame is there in memory, we move the frame to front of queue 
struct Page* LRU_get(int pageNumber)
{
	QNode* reqPage = hash->array[pageNumber];
	
	if (reqPage == NULL) return NULL;

	// page is there and not at front, change pointer 
	if (reqPage != queue->front) {
		// Unlink rquested page from its current location 
		// in queue. 
		reqPage->prev->next = reqPage->next;
		if (reqPage->next)
			reqPage->next->prev = reqPage->prev;

		// If the requested page is rear, then change rear 
		// as this node will be moved to front 
		if (reqPage == queue->rear) {
			queue->rear = reqPage->prev;
			queue->rear->next = NULL;
		}

		// Put the requested page before current front 
		reqPage->next = queue->front;
		reqPage->prev = NULL;

		// Change prev of current front 
		reqPage->next->prev = reqPage;

		// Change front to the requested page 
		queue->front = reqPage;
	}
	return reqPage->page;
}

void LRU_put(int pageNumber, char* data){
	if (pinned_page == queue->numberOfFrames) return;
	
	QNode* reqPage = hash->array[pageNumber];

	// the page is not in cache, bring it 
	if (reqPage == NULL){
		LRU_Enqueue(queue, hash, pageNumber, data);
	}
}

// A function to add a page with given 'pageNumber' to both queue 
// and hash 
void LRU_Enqueue(Queue* queue, Hash* hash, int pageNumber, char* data)
{
	// If all frames are full, remove the page at the rear 
	if (AreAllFramesFull(queue)) {
		// remove page from hash
		QNode *curr = queue->rear;
		while (curr != NULL &&  curr->page->isPinned != 0) curr = curr->prev;
		if(curr != NULL) hash->array[curr->pageNumber] = NULL;
		LRU_deQueue(queue, curr);
	}

	// Create a new node with given page number, 
	// And add the new node to the front of queue 
	QNode* temp = newQNode(pageNumber, data);
	temp->next = queue->front;

	// If queue is empty, change both front and rear pointers 
	if (isQueueEmpty(queue))
		queue->rear = queue->front = temp;
	else // Else change the front 
	{
		queue->front->prev = temp;
		queue->front = temp;
	}

	// Add page entry to hash also 
	hash->array[pageNumber] = temp;

	// increment number of full frames 
	queue->count++;
}

// A utility function to delete a frame from queue 
void LRU_deQueue(Queue* queue, QNode *curr)
{
	if (isQueueEmpty(queue) || curr == NULL)
		return;

	if (queue->front == queue->rear && curr == queue->front){
		queue->front = NULL;
		queue->rear = NULL;
	}
	else if (curr == queue->front){
		queue->front = queue->front->next;
		queue->front->prev = NULL;
	}
	else if (curr == queue->rear){
		queue->rear = queue->rear->prev;
		queue->rear->next = NULL;
	}
	else{
		curr->prev->next = curr->next;
		curr->next->prev = curr->prev;
	}

	if(curr->page->isDirty) evictedPage = curr->page;
	else
	    {
            free(curr->page);
            evictedPage = NULL;
	    }

	free(curr);

	// decrement the number of full frames by 1 
	queue->count--;
}

// A utility function to check if queue is empty 
int isQueueEmpty(Queue* queue)
{
	return queue->rear == NULL;
}

// A function to check if there is slot available in memory 
int AreAllFramesFull(Queue* queue)
{
	return queue->count == queue->numberOfFrames;
}

// A utility function to create an empty Hash of given capacity 
Hash* createHash(int capacity)
{
	// Allocate memory for hash 
	Hash* hash = (Hash*)malloc(sizeof(Hash));
	hash->capacity = capacity;

	// Create an array of pointers for refering queue nodes 
	hash->array = (QNode**)malloc(hash->capacity * sizeof(QNode*));

	// Initialize all hash entries as empty 
	int i;
	for (i = 0; i < hash->capacity; ++i)
		hash->array[i] = NULL;

	return hash;
}

// A utility function to create an empty Queue. 
// The queue can have at most 'numberOfFrames' nodes 
Queue* createQueue(int numberOfFrames)
{
	Queue* queue = (Queue*)malloc(sizeof(Queue));

	// The queue is empty 
	queue->count = 0;
	queue->front = queue->rear = NULL;

	// Number of frames that can be stored in memory 
	queue->numberOfFrames = numberOfFrames;

	return queue;
}

struct Page* findLRUPage(int pageNumber){
	QNode* reqPage = hash->array[pageNumber];
	if (reqPage == NULL) return NULL;
	return reqPage->page;
}

// A utility function to create a new Queue Node. The queue Node 
// will store the given 'pageNumber' 
QNode* newQNode(int pageNumber, char* data)
{
	// Allocate memory and assign 'pageNumber' 
	QNode* temp = (QNode*)malloc(sizeof(QNode));
	struct Page *page = (struct Page*)malloc(sizeof(struct Page));
	page->pageNumber = pageNumber;
	page->pageValue = data;
	page->isDirty = 0;
	page->isPinned = 0;
	page->fixCount = 0;
	temp->pageNumber = pageNumber;
	temp->page = page;
	// Initialize prev and next as NULL 
	temp->prev = temp->next = NULL;
	return temp;
}

void LRU_free(){
	QNode *front = queue->front;
	while (front != NULL){
		QNode *temp = front;
		front = front->next;
		if (temp->page != NULL) free(temp->page);
		free(temp);
	}
	hash = NULL; queue = NULL;
	free(hash);
}