#include <stdlib.h>
#include "dberror.h"
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "tables.h"
#include "Implement_btree.h"

char* createFilename(char *tableName);
int getNumPages(BTreeManager *tree);

// init and shutdown index manager
RC initIndexManager (void *mgmtData) { return RC_OK; }

RC shutdownIndexManager (){

    return RC_OK;
}

// create, destroy, open, and close an btree index
RC createBtree(char *idxId, DataType keyType, int count) {

    //Append '.bin' to idxID to create filename
    char *filename = createFilename(idxId);

    //Create a pagefile for this and store it
    RC response;
    response = createPageFile(filename);
    if (response != RC_OK) return response;

    //Init buffer pool to create new pages
    BM_BufferPool *bufferPool = (BM_BufferPool*) malloc(sizeof(BM_BufferPool));
    initBufferPool(bufferPool,filename,500,RS_LRU,NULL);

    //Write Index Header to page 1 of file
    BM_PageHandle *indexHeader = malloc(sizeof(BM_PageHandle));
    response = pinPage(bufferPool, indexHeader,1);
    if (response != RC_OK) return response;
    int order = count + 2; //TODO What value should this be?
    int num_nodes = 1;
    memcpy(indexHeader->data, &keyType,sizeof(DataType)); // DataType
    memcpy(indexHeader->data + sizeof(DataType), &order,sizeof(int)); // Number of nodes
    memcpy(indexHeader->data + sizeof(int)*2, &num_nodes,sizeof(int)); // Max element
    //num entries is 4th but 0 so no need to memcpy


    markDirty(bufferPool, indexHeader);
    response = unpinPage(bufferPool, indexHeader);
    if (response != RC_OK) return response;

    //Write Root node Page to page 2 of file
    BM_PageHandle *rootPage = malloc(sizeof(BM_PageHandle));

    response = pinPage(bufferPool, rootPage,2);
    if (response != RC_OK) return response;
    bool leaf = true;
    memcpy(rootPage->data + sizeof(int)*2, &leaf, sizeof(bool));
    markDirty(bufferPool, rootPage);
    response = unpinPage(bufferPool, rootPage);
    if (response != RC_OK) return response;

    //Free all memory
    free(filename);
    free(indexHeader->data);
    free(indexHeader);
    //free(rootPage->data);
    free(rootPage);
    response = shutdownBufferPool(bufferPool);

    return response;
}

RC openBtree (BTreeHandle **tree, char *idxId){

    (*tree) = malloc(sizeof(BTreeHandle));
    (*tree)->idxId = idxId;

    //Append '.bin' to idxID to create filename
    char *filename = createFilename(idxId);

    //Init buffer pool to create new pages
    RC response;
    BM_BufferPool *bufferPool = (BM_BufferPool*) malloc(sizeof(BM_BufferPool));
    initBufferPool(bufferPool,filename,500,RS_LRU,NULL);

    //Read index header page
    BM_PageHandle *indexHeader = malloc(sizeof(BM_PageHandle));
    response = pinPage(bufferPool,indexHeader,1);
    if (response != RC_OK) return response;

    //Load BtreeManager

    BTreeManager *btree_man = malloc(sizeof(BTreeManager));
    char tmp[4];

    btree_man->bufferPool = bufferPool;

    memcpy(tmp, indexHeader->data, sizeof(DataType)); // dt
    (*tree)->keyType = *(DataType *)tmp;

    memcpy(tmp, indexHeader->data + sizeof(int), sizeof(int)); // order
    btree_man->order = *(int *)tmp;

    memcpy(tmp, indexHeader->data + sizeof(int)*2, sizeof(int)); // numNodes
    btree_man->numNodes = *(int *)tmp;

    memcpy(tmp, indexHeader->data + sizeof(int)*3, sizeof(int)); // numEntries
    btree_man->numEntries = *(int *)tmp;

    (*tree)->mgmtData = btree_man;

    btree_man->numPages = getNumPages(btree_man);

    //Put index header page away
    response = unpinPage(bufferPool,indexHeader);
    if (response != RC_OK) return response;

    //Load Root Node
    TreeNode *root;
    getNodeFromPage(btree_man, &root, 2);
    btree_man->root = root;

    //Free data
    free(indexHeader->data);
    free(indexHeader);
    free(filename);

    return RC_OK;
}

RC closeBtree (BTreeHandle *tree){
    BTreeManager *btree_man = tree->mgmtData;

    //Update index header page
    BM_PageHandle *indexHeader = malloc(sizeof(BM_PageHandle));
    RC response;
    response = pinPage(btree_man->bufferPool,indexHeader,1);
    if (response != RC_OK) return response;

    memcpy(indexHeader->data, &tree->keyType, sizeof(DataType)); // order

    memcpy(indexHeader->data + sizeof(int), &btree_man->order, sizeof(int)); // order

    memcpy(indexHeader->data + sizeof(int)*2, &btree_man->numNodes, sizeof(int)); // numNodes

    memcpy(indexHeader->data + sizeof(int)*3, &btree_man->numEntries, sizeof(int)); // numEntries

    markDirty(btree_man->bufferPool, indexHeader);
    response = unpinPage(btree_man->bufferPool,indexHeader);
    if (response != RC_OK) return response;

    //Take out the garbage
    shutdownBufferPool(btree_man->bufferPool);
    freeNode(btree_man->root);
    free(btree_man);
    free(tree);

    return RC_OK;
}

RC deleteBtree (char *idxId){
    char *filename = createFilename(idxId);
    RC response = destroyPageFile(filename);
    free(filename);
    return response;
}

// access information about a b-tree
RC getNumNodes (BTreeHandle *tree, int *result){
    BTreeManager *btree_man = tree->mgmtData;
    if (btree_man == NULL) return RC_GENERIC_ERROR;
    *result = btree_man->numNodes;
    return RC_OK;
}

RC getNumEntries (BTreeHandle *tree, int *result){
    BTreeManager *btree_man = tree->mgmtData;
    if (btree_man == NULL) return RC_GENERIC_ERROR;
    *result = btree_man->numEntries;
    return RC_OK;
}

RC getKeyType (BTreeHandle *tree, DataType *result) {
    *result = tree->keyType;
    return RC_OK;
}

RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle){
    (*handle) = malloc(sizeof(BT_ScanHandle));
    (*handle)->tree = tree;
    BT_ScanCurrEntry *ce = malloc(sizeof(BT_ScanCurrEntry));
    BTreeManager *bman = tree->mgmtData;

    TreeNode *tmp = bman->root;

    while (!tmp->isLeaf) getNodeFromPage(bman, &tmp, getPageNum(tmp->pointer[0]));

    ce->currNode = tmp;
    ce->currEntry = 0;

    return RC_OK;
}

RC nextEntry(BT_ScanHandle *handle, RID *result){
    BT_ScanCurrEntry *ce = handle->mgmtData;
    BTreeManager *bman = handle->tree->mgmtData;

    if (ce->currEntry >= ce->currNode->numofkeys) {
        if (ce->currNode->next == 0) return RC_IM_NO_MORE_ENTRIES;
        getNodeFromPage(bman, &ce->currNode, ce->currNode->next);
        ce->currEntry = 0;
    }

    result->page = ce->currNode->pageNum;
    result->slot = ce->currEntry;

    ce->currEntry++;

    return RC_OK;
}

RC closeTreeScan (BT_ScanHandle *handle) {

    BT_ScanCurrEntry *ce = handle->mgmtData;

    if (ce->currNode != NULL) freeNode(ce->currNode);
    free(ce);
    free(handle);

    return RC_OK; }

char* createFilename(char *tableName) {
    char* filename = malloc(sizeof(char)*strlen(tableName)+sizeof(char)*4+1);
    strcpy(filename,tableName);
    strcat(filename,".bin");
    return filename;
}



int getNumPages(BTreeManager *tree) {

    //Read the page with the node
    RC response;
    BM_PageHandle *nodePage = malloc(sizeof(BM_PageHandle));
    response = pinPage(tree->bufferPool,nodePage,0);
    if (response != RC_OK) return response;

    int tmp[4];
    memcpy(tmp, nodePage->data, sizeof(int));

    response = unpinPage(tree->bufferPool,nodePage);
    if (response != RC_OK) return response;

    free(nodePage->data);
    free(nodePage);

    return *(int *)tmp;
}


RC insertKey(BTreeHandle *tree, Value *key, RID rid) {

    //printf("insertKey Begin\n");
    BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
    NodeData *pointer;
    TreeNode *leaf;

    int order = treeManager->order;

    if (searchRecord(treeManager, treeManager->root, key) != NULL) {
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    pointer = GenerateRecord(&rid);

    /*if (treeManager->root == NULL) {
        treeManager->root = createNewBTree(treeManager, key, pointer);
        return RC_OK;
    }*/

    leaf = searchLeaf(treeManager, treeManager->root, key);

    if (leaf->numofkeys < order - 1) {
        leaf = InsertLeaf(treeManager, leaf, key, pointer);
    } else {
        treeManager->root = InsertLeafAfterSplit(treeManager, leaf, key, pointer);
    }

    //printf("deleteBtree End\n");
    return RC_OK;
}

extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {

    //printf("findKey Begin\n");
    BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
    NodeData *record = searchRecord(treeManager, treeManager->root, key);
    if (record == NULL) {
        return RC_IM_KEY_NOT_FOUND;
    } else {}

    *result = record->rid;
    //printf("findKey End\n");
    return RC_OK;
}



RC deleteKey(BTreeHandle *tree, Value *key) {
    //printf("deleteKey Begin\n");
    BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
    treeManager->root = DeleteNode(treeManager, key);
    //printf("deleteKey End\n");
    return RC_OK;
}



/*

extern char *printTree(BTreeHandle *tree) {

    //printf("Print Tree Being \n");
    BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
    //printf("\nPRINTING TREE:\n");
    TreeNode *node = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (treeManager->root == NULL) {
        //printf("Empty tree.\n");
        return '\0';
    }
    treeManager->queue = NULL;
    enqueue(treeManager, treeManager->root);
    while (treeManager->queue != NULL) {
        node = dequeue(treeManager);
        if (node->parent != NULL && node == node->parent->pointer[0]) {
            new_rank = RootPath(treeManager->root, node);
            if (new_rank != rank) {
                rank = new_rank;
                printf("\n");
            }
        }

        for (i = 0; i < node->numofkeys; i++) {
            switch (treeManager->keyType) {
                case DT_INT:
                    printf("%d ", (*node->keys[i]).v.intV);
                    break;
                case DT_FLOAT:
                    printf("%.02f ", (*node->keys[i]).v.floatV);
                    break;
                case DT_STRING:
                    printf("%s ", (*node->keys[i]).v.stringV);
                    break;
                case DT_BOOL:
                    printf("%d ", (*node->keys[i]).v.boolV);
                    break;
            }
            printf("(%d - %d) ", ((NodeData *) node->pointer[i])->rid.page, ((NodeData *) node->pointer[i])->rid.slot);
        }
        if (!node->isLeaf) {
            for (i = 0; i <= node->numofkeys; i++) {
                enqueue(treeManager, node->pointer[i]);
            }
        }

        printf(" | ");
    }
    printf("\n");

    return '\0';
}*/