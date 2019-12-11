#include "strategy.h"

extern Queue* queue;
extern struct Node* front;
Page* evictedPage = NULL;
int strategy_type = -1;
int pinned_page = 0;

//Initialize one strategy.
void init(int type, int capacity, int hashSize){
	if (strategy_type != -1) return;
    strategy_type = type;
	if (type == 0) initFIFO(capacity, hashSize);
	else if (type == 1) initLRU(capacity, hashSize);
}

Page *get(int pageNum) {
	if (strategy_type == -1) return NULL;
	if (strategy_type == 0) return FIFO_get(pageNum);
	else if (strategy_type == 1) return LRU_get(pageNum);
	return NULL;
}

Page* put(int pageNum, char* pageContent){
	if (strategy_type == -1) return NULL;
	if (strategy_type == 0){
        FIFO_put(pageNum, pageContent);
	}
	else if (strategy_type == 1){
        LRU_put(pageNum, pageContent);
	}
    if(evictedPage != NULL){
        Page* page = evictedPage;
        evictedPage = NULL;
        return page;
    }
	return NULL;
}

Page *find(int pageNum) {
	if (strategy_type == -1) return NULL;
	if (strategy_type == 0) return FIFO_get(pageNum);
	if (strategy_type == 1) return findLRUPage(pageNum);
}

RC dirty(int pageNumber) {
    Page *page = find(pageNumber);
    if (page == NULL) return RC_IM_KEY_NOT_FOUND;
    page->isDirty = 1;
    return RC_OK;
}

RC clean(int pageNumber) {
    Page *page = find(pageNumber);
    if (page == NULL) return RC_IM_KEY_NOT_FOUND;
    page->isDirty = 0;
    return RC_OK;
}

RC pin(int pageNumber) {
    Page *page = find(pageNumber);
    if (page == NULL) return RC_IM_KEY_NOT_FOUND;
    pinned_page++;
    page->fixCount++;
    page->isPinned = 1;
    return RC_OK;
}

RC unpin(int pageNumber) {
    Page *page = find(pageNumber);
    if (page == NULL) return RC_IM_KEY_NOT_FOUND;
    pinned_page--;
    if (page->fixCount > 0) page->fixCount--;
    if (page->fixCount == 0) page->isPinned = 0;
    return RC_OK;
}

struct Queue* getLRUBuffer(){
    return queue;
};

struct Node* getFIFOBuffer(){
    return front;
};

void freeALL(){
	if (strategy_type == -1) return;
	if (strategy_type == 0){
		FIFO_free();
	}
	else if (strategy_type == 1){
		LRU_free();
	}
    strategy_type = -1;
    pinned_page = 0;
}