#ifdef __cplusplus
extern "C" {
#endif
#include "btree_mgr.h"
#include "buffer_mgr.h"

// Structure to store node data
typedef struct TreeNode {
    int pageNum;
    void **pointer;
    Value **keys;
    int parent;
    bool isLeaf;
    int numofkeys;
    int next;
} TreeNode;
// B+ Tree Manager
typedef struct BTreeManager {
    BM_BufferPool *bufferPool;
    int order;
    int numNodes;
    int numEntries;
    int numPages;
    TreeNode *root;
    TreeNode *queue;
    DataType keyType;
} BTreeManager;

typedef struct NodeData {
    RID rid;
} NodeData;

typedef struct BT_ScanCurrEntry {
    TreeNode *currNode;
    int currEntry;

} BT_ScanCurrEntry;
RC getNodeFromPage(BTreeManager *tree, TreeNode **node, int pageNum);
RC writeNodeToPage(BTreeManager *tree, TreeNode *node);
RC freeNode(TreeNode *node);
int getPageNum(void * pointer);
// Search Functions
TreeNode *searchLeaf(BTreeManager *BTreeNode, TreeNode *root, Value *key);
NodeData *searchRecord(BTreeManager *BTreeNode, TreeNode *root, Value *key);

//Helper functions
void enqueue(BTreeManager *BTreeNode, TreeNode *newnode);
TreeNode *dequeue(BTreeManager *BTreeNode);
int RootPath(BTreeManager *BTreeNode, TreeNode *root, TreeNode *child);

// Insertion functions
NodeData *GenerateRecord(RID *rid);
TreeNode *InsertLeaf(BTreeManager *BTreeNode, TreeNode *leaf, Value *key, NodeData *pointer);
TreeNode *createNewBTree(BTreeManager *BTreeNode, Value *key, NodeData *pointer);

TreeNode *createNode(BTreeManager *BTreeNode);

TreeNode *createLeaf(BTreeManager *BTreeNode);
TreeNode *InsertLeafAfterSplit(BTreeManager *BTreeNode, TreeNode *leaf, Value *key, void * pointer);
TreeNode *InsertNode(BTreeManager *BTreeNode, TreeNode *parent, int left_index, Value *key, TreeNode *right);
TreeNode *InsertNodeAfterSplit(BTreeManager *BTreeNode, TreeNode *oldNode, int left_index, Value *key, TreeNode *right);
TreeNode *InsertParent(BTreeManager *BTreeNode, TreeNode *left, Value *key, TreeNode *right);
TreeNode *InsertNewRoot(BTreeManager *BTreeNode, TreeNode *left, Value *key, TreeNode *right);
int GetLeftIndex(TreeNode *parent, TreeNode *left);

// Deletion Functions
TreeNode *ModifyRoot(BTreeManager *BTreeNode, TreeNode *root);

TreeNode *mergeNodes(BTreeManager *BTreeNode, TreeNode *nodee, TreeNode *neighbor, int neighbor_index, Value *k_prime);

TreeNode *
ReBalanceNodes(BTreeManager *BTreeNode, TreeNode *root, TreeNode *n, TreeNode *neighbor, int neighbor_index, int k_prime_index, Value *k_prime);

TreeNode *DeleteEntry(BTreeManager *BTreeNode, TreeNode *nodee, Value *key, void * pointer);

TreeNode *DeleteNode(BTreeManager *BTreeNode, Value *key);

TreeNode *RemoveEntryOfNode(BTreeManager *BTreeNode, TreeNode *n, Value *key, void * pointer);

int getNeighborIndex(BTreeManager *BTreeNode, TreeNode *nodee);

// Handle Keys
bool isLess(Value *key1, Value *key2);

bool isGreater(Value *key1, Value *key2);

bool isEqual(Value *key1, Value *key2);


#ifdef __cplusplus
}
#endif