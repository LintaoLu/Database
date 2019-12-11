//
// Created by jaime on 9/26/19.
//

#include <buffer_mgr.h>

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy thisStrategy,
                  void *stratData) { // I don't know what stratData is

    SM_FileHandle *fileInfo = malloc(1 * sizeof(SM_FileHandle));
    int response = openPageFile((char *) pageFileName, fileInfo);
    if (response != RC_OK) {
        bm->mgmtData = NULL;
        return response;
    }

    bm->pageFile = pageFileName; //We really need the page handle not the filename but ok
    bm->numPages = numPages;
    bm->strategy = thisStrategy;
    BM_Mgmt_Info *bi = malloc(1 * sizeof(BM_Mgmt_Info));

    bi->fHandle = fileInfo; //We need this to be able to close and write the file based on the storage manage functions
    bi->numWriteIO = 0;
    bi->numReadIO = 0;

    bm->mgmtData = bi;

    //size of hash table should be bigger....
    init(thisStrategy, numPages, 100000);

    return RC_OK;
}

//Can only shut down a pool if no pages are pinned
RC shutdownBufferPool(BM_BufferPool *const bm) {
    BM_Mgmt_Info *bi = (BM_Mgmt_Info *) bm->mgmtData;

    if (bi == NULL || bi->fHandle->mgmtInfo == NULL) return RC_IM_KEY_NOT_FOUND;
    //Cannot shutdown BM if any page is pinned by any clients
    if (anyPagePinned(bm)) {
        return RC_PINNED_PAGES;
    }

    //flush all dirty pages
    int response = forceFlushPool(bm);
    if (response != RC_OK) {
        return response;
    }

    freeALL();

    closePageFile(bi->fHandle);
    free(bi->fHandle);
    free(bi);
    free(bm);

    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm) {
    BM_Mgmt_Info *bi = (BM_Mgmt_Info *) bm->mgmtData;
    if (bi == NULL) return RC_IM_KEY_NOT_FOUND;

    BM_PageHandle *ph = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));
    //Find all the dirty pages with fipageNumx count 0 and write them to the file
    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        while (node != NULL) {
            if (node->page->isDirty){
                ph->pageNum = node->page->pageNumber;
                ph->data = node->page->pageValue;
                forcePage(bm, ph);
            }
            node = node->next;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        while (node != NULL) {
            if (node->page->isDirty){
                ph->pageNum = node->page->pageNumber;
                ph->data = node->page->pageValue;
                forcePage(bm, ph);
            }
            node = node->next;
        }
    }
    else{
        //Probably need an error for this case
    }

    free(ph);
    return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    return dirty(page->pageNum);
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    return unpin(page->pageNum);
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) { //Needs to increment writeIOcount
    BM_Mgmt_Info *bi = (BM_Mgmt_Info *) bm->mgmtData;
    if (bi == NULL) return RC_IM_KEY_NOT_FOUND;

    Page *pageData = get(page->pageNum);

    if (pageData == NULL || pageData->pageValue == NULL)
        return RC_IM_KEY_NOT_FOUND;

    if (writeBlock(page->pageNum, bi->fHandle, pageData->pageValue) == RC_OK) {
        bi->numWriteIO++;
        clean(page->pageNum);
        return RC_OK;
    }
    return RC_WRITE_FAILED;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
           const PageNumber pageNum) {
    BM_Mgmt_Info *bi = (BM_Mgmt_Info *) bm->mgmtData;
    if (bi == NULL) return RC_IM_KEY_NOT_FOUND;

    page->pageNum = pageNum;
    page->data = malloc(sizeof(char)*PAGE_SIZE);
    Page *foundPage = find(pageNum);
    if (foundPage != NULL) {
        pin(page->pageNum);
        page->data = get(page->pageNum)->pageValue;
        return RC_OK;
    }

    if (pageNum > bi->fHandle->totalNumPages) {
        if (ensureCapacity(pageNum, bi->fHandle) != RC_OK) {
            return RC_WRITE_FAILED;
        }
    }

    if (readBlock(page->pageNum, bi->fHandle,  page->data) == RC_OK) {
        bi->numReadIO++;
        Page *evictedPage = put(page->pageNum, page->data);
        pin(page->pageNum);
        if (evictedPage != NULL) {
            if (evictedPage->pageNumber == -1) {
                free(evictedPage);
                return RC_PINNED_PAGES;
            }
            if (writeBlock(evictedPage->pageNumber, bi->fHandle, evictedPage->pageValue) == RC_OK) {
                bi->numWriteIO++;
                free(evictedPage);
                return RC_OK;
            }
        }
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm) {
    PageNumber *frameContents = malloc(bm->numPages * sizeof(PageNumber));
    memset(frameContents, NO_PAGE, sizeof(PageNumber) * bm->numPages);

    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        int i = 0;
        while (node != NULL) {
            frameContents[i] = node->page->pageNumber;
            node = node->next;
            i++;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        int i = 0;
        while (node != NULL) {
            frameContents[i] = node->page->pageNumber;
            node = node->next;
            i++;
        }
    }
    else{
        //Probably need an error for this case
    }
    return frameContents;
}

bool *getDirtyFlags(BM_BufferPool *const bm) {
    bool *dirtyFlags = malloc(bm->numPages * sizeof(bool));
    memset(dirtyFlags, false, sizeof(bool) * bm->numPages);

    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        int i = 0;
        while (node != NULL) {
            dirtyFlags[i] = node->page->isDirty; //Assuming Lu changed it to bool
            node = node->next;
            i++;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        int i = 0;
        while (node != NULL) {
            dirtyFlags[i] = node->page->isDirty; //Assuming Lu changed it to bool
            node = node->next;
            i++;
        }
    }
    else{
        //Probably need an error for this case
    }
    return dirtyFlags;
}

int *getFixCounts(BM_BufferPool *const bm) {
    int *fixCounts = malloc(bm->numPages * sizeof(int));
    memset(fixCounts, 0, sizeof(int) * bm->numPages);

    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        int i = 0;
        while (node != NULL) {
            fixCounts[i] = node->page->fixCount; //Assuming Lu changed isPinned to fixCount
            node = node->next;
            i++;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        int i = 0;
        while (node != NULL) {
            fixCounts[i] = node->page->fixCount; //Assuming Lu changed isPinned to fixCount
            node = node->next;
            i++;
        }
    }
    else{
        //Probably need an error for this case
    }
    return fixCounts;
}

int getNumReadIO(BM_BufferPool *const bm) {

    BM_Mgmt_Info* bi = (BM_Mgmt_Info*)bm->mgmtData;

    return bi->numReadIO;
}

int getNumWriteIO(BM_BufferPool *const bm) {

    BM_Mgmt_Info* bi = (BM_Mgmt_Info*)bm->mgmtData;

    return bi->numWriteIO;
}

bool anyPagePinned(BM_BufferPool *const bm) {
    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        while (node != NULL) {
            if (node->page->fixCount > 0) return true; //Assuming Lu changed isPinned to fixCount
            node = node->next;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        while (node != NULL) {
            if (node->page->fixCount > 0) return true; //Assuming Lu changed isPinned to fixCount
            node = node->next;
        }
    }
    else{
        //Probably need an error for this case
    }
    return false;
}

void unPinAll(BM_BufferPool *const bm) {
    if (bm->strategy == RS_LRU) {
        QNode *node = getLRUBuffer()->front;
        while (node != NULL) {
            if (node->page->isPinned) {
                BM_PageHandle *memPage = malloc(sizeof(BM_PageHandle));
                memPage->data = malloc(PAGE_SIZE* sizeof(char));
                memPage->pageNum = node->page->pageNumber;
                unpinPage(bm,memPage);
            }
            node = node->next;
        }
    } else if (bm->strategy == RS_FIFO) {
        Node *node = getFIFOBuffer();
        while (node != NULL) {
            if (node->page->isPinned) {
                BM_PageHandle *memPage = malloc(sizeof(BM_PageHandle));
                memPage->data = malloc(PAGE_SIZE* sizeof(char));
                memPage->pageNum = node->page->pageNumber;
                unpinPage(bm,memPage);
            }
            node = node->next;
        }
    }
    else{
        //Probably need an error for this case
    }
}