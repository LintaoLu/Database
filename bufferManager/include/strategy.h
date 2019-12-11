#ifndef STRATEGY_H
#define STRATEGY_H

#include <stdlib.h>
#include <dberror.h>
#include <stdio.h>
#include "FIFO.h"
#include "LRU.h"
#include "Page.h"

void init(int type, int capacity, int hashSize);
struct Page* get(int pageNum);
Page* put(int pageNum, char* pageContent);
struct Queue* getLRUBuffer();
struct Node* getFIFOBuffer();
struct Page* find(int pageNumber);

RC unpin(int pageNumber);

RC pin(int pageNumber);

RC clean(int pageNumber);

RC dirty(int pageNumber);
void freeALL();

#endif