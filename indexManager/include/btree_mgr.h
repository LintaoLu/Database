#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "dberror.h"
#include "tables.h"
#include "btree_mgr.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// structure for accessing btrees
typedef struct BTreeHandle {
    DataType keyType;
    char *idxId;
    void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
    BTreeHandle *tree;
    void *mgmtData;
} BT_ScanHandle;





// init and shutdown index manager
EXTERNC RC initIndexManager(void *mgmtData);

EXTERNC RC shutdownIndexManager();

// create, destroy, open, and close an btree index
EXTERNC RC createBtree(char *idxId, DataType keyType, int n);

EXTERNC RC openBtree(BTreeHandle **tree, char *idxId);

EXTERNC RC closeBtree(BTreeHandle *tree);

EXTERNC RC deleteBtree(char *idxId);

// access information about a b-tree
EXTERNC RC getNumNodes(BTreeHandle *tree, int *result);

EXTERNC RC getNumEntries(BTreeHandle *tree, int *result);

EXTERNC RC getKeyType(BTreeHandle *tree, DataType *result);

// index access
EXTERNC RC findKey(BTreeHandle *tree, Value *key, RID *result);

EXTERNC RC insertKey(BTreeHandle *tree, Value *key, RID rid);

EXTERNC RC deleteKey(BTreeHandle *tree, Value *key);

EXTERNC RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle);

EXTERNC RC nextEntry(BT_ScanHandle *handle, RID *result);

EXTERNC RC closeTreeScan(BT_ScanHandle *handle);

// debug and test functions
EXTERNC char *printTree(BTreeHandle *tree);

//


#endif // BTREE_MGR_H