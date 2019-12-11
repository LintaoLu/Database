#include <stdlib.h>
#include "Implement_btree.h"
#include "dt.h"
#include "string.h"

/**
 * This function Creates a new record (NodeData) to hold the value to which a key refers.
 * @param rid
 * @return record structure
 */
NodeData *GenerateRecord(RID *rid) {
    printf("GenerateRecord Begin\n");
    NodeData *record = (NodeData *) malloc(sizeof(NodeData));
    if (record == NULL) {
        perror("NodeData creation.");
        exit(RC_GENERIC_ERROR);
    } else {
        record->rid.page = rid->page;
        record->rid.slot = rid->slot;
    }
    printf("GenerateRecord End\n");
    return record;
}



/**
 * This function Insert new pointer to record and corresponding key into leaf
 * @param BTreeNode
 * @param leaf
 * @param key
 * @param pointer
 * @return Modified leaf
 */
TreeNode *InsertLeaf(BTreeManager *BTreeNode, TreeNode *leaf, Value *key, NodeData *pointer) {
    printf("InsertLeaf Begin\n");
    int i, WheretoInsert;
    BTreeNode->numEntries++;
    WheretoInsert = 0;
    while (WheretoInsert < leaf->numofkeys && isLess(leaf->keys[WheretoInsert], key)) {
        WheretoInsert++;
    }
    for (i = leaf->numofkeys; i > WheretoInsert; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointer[i] = leaf->pointer[i - 1];
    }
    leaf->keys[WheretoInsert] = key;
    leaf->pointer[WheretoInsert] = pointer;
    leaf->numofkeys++;
    writeNodeToPage(BTreeNode, leaf);
    printf("InsertLeaf End\n");
    return leaf;
}

/**
 * This function inserts new key and pointer to new record into leaf node,splitting the leaf
 * @param BTreeNode
 * @param leaf
 * @param key
 * @param pointer
 * @return updated tree with new parent
 */
TreeNode *InsertLeafAfterSplit(BTreeManager *BTreeNode, TreeNode *leaf, Value *key, void * pointer) {

    printf("Begin InsertLeafAfterSplit\n");
    TreeNode *newLeaf;
    Value **TempKeys;
    void **TempPointer;
    int IIndex, split, i, j;
    Value *NewKey;
    newLeaf = createLeaf(BTreeNode);
    BTreeNode->numPages++;
    BTreeNode->numNodes++;
    newLeaf->pageNum = BTreeNode->numPages;

    int order = BTreeNode->order;

    TempKeys = malloc(order * sizeof(Value));
    if (TempKeys == NULL) {
        perror("Temporary keys array.");
        exit(RC_GENERIC_ERROR);
    }

    TempPointer = malloc(order * sizeof(int));
    if (TempPointer == NULL) {
        perror("Temporary pointer array.");
        exit(RC_GENERIC_ERROR);
    }
    IIndex = 0;
    while (IIndex < order - 1 && isLess(leaf->keys[IIndex], key)) {
        IIndex++;
    }
    for (i = 0, j = 0; i < leaf->numofkeys; i++, j++) {
        if (j == IIndex)
            j++;
        TempKeys[j] = leaf->keys[i];
        TempPointer[j] = leaf->pointer[i];
    }

    TempKeys[IIndex] = key;
    TempPointer[IIndex] = pointer;

    leaf->numofkeys = 0;

    if ((order - 1) % 2 == 0)
        split = (order - 1) / 2;
    else
        split = (order - 1) / 2 + 1;
    for (i = 0; i < split; i++) {
        leaf->pointer[i] = TempPointer[i];
        leaf->keys[i] = TempKeys[i];
        leaf->numofkeys++;
    }
    for (i = split, j = 0; i < order; i++, j++) {
        newLeaf->pointer[j] = TempPointer[i];
        newLeaf->keys[j] = TempKeys[i];
        newLeaf->numofkeys++;
    }

    free(TempPointer);
    free(TempKeys);
    newLeaf->pointer[order - 1] = leaf->pointer[order - 1];
    leaf->pointer[order - 1] = &newLeaf->pageNum;

    for (i = leaf->numofkeys; i < order - 1; i++)
        leaf->pointer[i] = 0;
    for (i = newLeaf->numofkeys; i < order - 1; i++)
        newLeaf->pointer[i] = 0;

    newLeaf->parent = leaf->parent;
    NewKey = newLeaf->keys[0];
    BTreeNode->numEntries++;

    writeNodeToPage(BTreeNode,newLeaf);
    writeNodeToPage(BTreeNode,leaf);

    printf("InsertLeafAfterSplit End\n");
    return InsertParent(BTreeNode, leaf, NewKey, newLeaf);
}

/**
 * This function inserts new key and pointer to node into non-leaf node, splitting the node
 * @param BTreeNode
 * @param oldNode
 * @param left_index
 * @param key
 * @param right
 * @return updated tree.
 */
TreeNode *
InsertNodeAfterSplit(BTreeManager *BTreeNode, TreeNode *oldNode, int left_index, Value *key, TreeNode * right) {

    printf("InsertNodeAfterSplit Begin\n");
    int i, j, split;
    Value *k_prime;
    TreeNode *newNode, *child;
    Value **tempKeys;
    void **tempPointers;

    int order = BTreeNode->order;

    tempPointers = malloc((order + 1) * sizeof(TreeNode *));
    if (tempPointers == NULL) {
        perror("Temporary pointer array for splitting nodes.");
        exit(RC_GENERIC_ERROR);
    }
    tempKeys = malloc(order * sizeof(Value *));
    if (tempKeys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(RC_GENERIC_ERROR);
    }

    for (i = 0, j = 0; i < oldNode->numofkeys + 1; i++, j++) {
        if (j == left_index + 1)
            j++;
        tempPointers[j] = oldNode->pointer[i];
    }

    for (i = 0, j = 0; i < oldNode->numofkeys; i++, j++) {
        if (j == left_index)
            j++;
        tempKeys[j] = oldNode->keys[i];
    }

    tempPointers[left_index + 1] = &right->pageNum;
    tempKeys[left_index] = key;
    if ((order - 1) % 2 == 0)
        split = (order - 1) / 2;
    else
        split = (order - 1) / 2 + 1;

    newNode = createNode(BTreeNode);
    BTreeNode->numPages++;
    BTreeNode->numNodes++;
    newNode->pageNum = BTreeNode->numPages;
    oldNode->numofkeys = 0;
    for (i = 0; i < split - 1; i++) {
        oldNode->pointer[i] = tempPointers[i];
        oldNode->keys[i] = tempKeys[i];
        oldNode->numofkeys++;
    }
    oldNode->pointer[i] = tempPointers[i];
    k_prime = tempKeys[split - 1];
    for (++i, j = 0; i < order; i++, j++) {
        newNode->pointer[j] = tempPointers[i];
        newNode->keys[j] = tempKeys[i];
        newNode->numofkeys++;
    }
    newNode->pointer[j] = tempPointers[i];
    free(tempPointers);
    free(tempKeys);
    newNode->parent = oldNode->parent;
    for (i = 0; i <= newNode->numofkeys; i++) {
        getNodeFromPage(BTreeNode, &child, getPageNum(newNode->pointer[i]));
        child->parent = newNode->pageNum;
        writeNodeToPage(BTreeNode, child);
    }
    BTreeNode->numEntries++;

    writeNodeToPage(BTreeNode, newNode);
    writeNodeToPage(BTreeNode, oldNode);

    printf("InsertNodeAfterSplit End\n");
    return InsertParent(BTreeNode, oldNode, k_prime, newNode);
}

/**
 * Insert new node in B+ Tree considering the following cases: In case the node is a leaf or node, find the parent's pointer to the left node.
 * then we see if the new key can accommodate in the node. If not we split node.
 * @param BTreeNode
 * @param left
 * @param key
 * @param right
 * @return Tree root
 */
TreeNode *InsertParent(BTreeManager *BTreeNode, TreeNode *left, Value *key, TreeNode *right) {
    printf("InsertParent Begin");
    int left_index;
    int parent = left->parent;
    TreeNode *parentNode;

    int order = BTreeNode->order;

    if (parent == 0) {
        return InsertNewRoot(BTreeNode, left, key, right);
    }
    getNodeFromPage(BTreeNode, &parentNode, left->parent );
    left_index = GetLeftIndex(parentNode, left);

    if (parentNode->numofkeys < order - 1) {
        return InsertNode(BTreeNode, parentNode, left_index, key, right);
    }
    printf("InsertParent End\n");
    return InsertNodeAfterSplit(BTreeNode, parentNode, left_index, key, right);
}

/**
 * Obtain the left Index
 * @param parent
 * @param left
 * @return left_index
 */
int GetLeftIndex(TreeNode *parent, TreeNode *left) {
    printf("GetLeftIndex Begin\n");
    int left_index = 0;
    while (left_index <= parent->numofkeys && parent->pointer[left_index] != &left->pageNum)
        left_index++;
    printf("GetLeftIndex End\n");
    return left_index;
}

/**
 *Insert key and pointer to node into B+ tree
 * @param BTreeNode
 * @param parent
 * @param left_index
 * @param key
 * @param right
 * @return root of insert updated tree
 */
TreeNode *InsertNode(BTreeManager *BTreeNode, TreeNode *parent, int left_index, Value *key, TreeNode *right) {
    printf("InsertNode Begin\n");
    int i;
    for (i = parent->numofkeys; i > left_index; i--) {
        parent->pointer[i + 1] = parent->pointer[i];
        parent->keys[i] = parent->keys[i - 1];
    }
    parent->pointer[left_index + 1] = &right->pageNum;
    parent->keys[left_index] = key;
    parent->numofkeys++;
    printf("InsertNode End\n");
    return BTreeNode->root;
}

/**
 * Create new root for two subtrees and insert appropriate key into new root
 * @param BTreeNode
 * @param left
 * @param key
 * @param right
 * @return updated root
 */
TreeNode *InsertNewRoot(BTreeManager *BTreeNode, TreeNode *left, Value *key, TreeNode *right) {
    printf("InsertNewRoot Begin\n");
    TreeNode *root = createNode(BTreeNode);
    root->pageNum = 2;
    root->keys[0] = key;
    root->pointer[0] = &left->pageNum;
    root->pointer[1] = &right->pageNum;
    root->numofkeys++;
    root->parent = 0;
    left->parent = root->pageNum;
    right->parent = root->pageNum;
    writeNodeToPage(BTreeNode, root);
    writeNodeToPage(BTreeNode, left);
    writeNodeToPage(BTreeNode, right);
    printf("InsertNewRoot End\n");
    return root;
}
/**
 * Create new node. It can be used as leaf or internal node.
 * @param BTreeNode
 * @return a new node
 */
TreeNode *createNode(BTreeManager *BTreeNode) {
    printf("createNode Begin\n");

    int order = BTreeNode->order;
    TreeNode *newNode = malloc(sizeof(TreeNode));
    if (newNode == NULL) {
        perror("TreeNode creation.");
        exit(RC_GENERIC_ERROR);
    }
    newNode->keys = (Value **) malloc((order - 1) * sizeof(Value *));
    if (newNode->keys == NULL) {
        perror("New node keys array.");
        exit(RC_GENERIC_ERROR);
    }
    newNode->pointer = malloc(order * sizeof(void *));
    if (newNode->pointer == NULL) {
        perror("New node pointer array.");
        exit(RC_GENERIC_ERROR);
    }
    newNode->isLeaf = false;
    newNode->numofkeys = 0;
    newNode->parent = 0;
    newNode->next = 0;
    printf("createNode End\n");
    return newNode;
}

/**
 * Create a new leaf by creating a node.
 * @param treeManager
 * @return leaf node
 */
TreeNode *createLeaf(BTreeManager *treeManager) {
    printf("createLeaf Start\n");
    TreeNode *leaf = createNode(treeManager);
    leaf->isLeaf = true;
    printf("createLeaf End\n");
    return leaf;
}

/**
 * Search for key in entire Tree
 * @param root
 * @param key
 * @return leaf containing the given key
 */
TreeNode *searchLeaf(BTreeManager *treeManager, TreeNode *root, Value *key) {
    printf("searchLeaf Begin\n");
    int i = 0;
    TreeNode *c = root;
    if (c == NULL) {
        return c;
    }
    while (!c->isLeaf) {
        i = 0;
        while (i < c->numofkeys) {
            if (isGreater(key, c->keys[i]) || isEqual(key, c->keys[i])) {
                i++;
            } else
                break;
        }
        getNodeFromPage(treeManager, &c, getPageNum(c->pointer[i]));
    }
    printf("searchLeaf End\n");
    return c;
}

/**
 * Finds record which a key refers
 * @param root
 * @param key
 * @return Returns record which a key refers
 */
NodeData *searchRecord(BTreeManager *treeManager, TreeNode *root, Value *key) {
    printf("searchRecord Begin\n");
    int i = 0;
    TreeNode *c = searchLeaf(treeManager, root, key);
    if (c == NULL) return NULL;
    for (i = 0; i < c->numofkeys; i++) {
        if (isEqual(c->keys[i], key))
            break;
    }
    printf("searchRecord End\n");
    if (i == c->numofkeys)
        return NULL;
    else {
        getNodeFromPage(treeManager, &c, getPageNum(c->pointer[i]));
        return (NodeData *) c;
    }

}

/**
 * Get the Neighbour Index
 * @param nodee
 * @return index of a node's nearest neighbor to the left if one exists else -1
 */
int getNeighborIndex(BTreeManager *treeManager, TreeNode *nodee) {
    printf("getNeighborIndex Begin\n");

    TreeNode *parentNode;
    getNodeFromPage(treeManager, &parentNode, nodee->parent);

    int i;
    for (i = 0; i <= parentNode->numofkeys; i++) {
        if (parentNode->pointer[i] == &nodee->pageNum) return i - 1;
    }
    printf("getNeighborIndex End\n");

    return RC_GENERIC_ERROR;
}

/**
 * Rmove record having the specified key from the the specified node.
 * @param BTreeNode
 * @param n
 * @param key
 * @param pointer
 * @return updated node
 */
TreeNode *RemoveEntryOfNode(BTreeManager *BTreeNode, TreeNode *n, Value *key, void * pointer) {

    printf("RemoveEntryOfNode Begin\n");
    int i, numPointers;
    int bTreeOrder = BTreeNode->order;
    i = 0;
    while (!isEqual(n->keys[i], key)) {
        i++;
    }

    for (++i; i < n->numofkeys; i++) {
        n->keys[i - 1] = n->keys[i];
    }
    numPointers = n->isLeaf ? n->numofkeys : n->numofkeys + 1;
    i = 0;
    while (n->pointer[i] != &pointer) { i++; }
    for (++i; i < numPointers; i++) {
        n->pointer[i - 1] = n->pointer[i];
    }
    n->numofkeys--;
    BTreeNode->numEntries--;

    if (n->isLeaf)
        for (i = n->numofkeys; i < bTreeOrder - 1; i++)
            n->pointer[i] = 0;
    else
        for (i = n->numofkeys + 1; i < bTreeOrder; i++)
            n->pointer[i] = 0;

    printf("RemoveEntryOfNode End\n");
    return n;
}
/**
 * Modify root after a record has been deleted
 * @param root
 * @return newRoot
 */
TreeNode *ModifyRoot(BTreeManager *BTreeNode, TreeNode *root) {
    printf("ModifyRoot Begin\n");
    TreeNode *newRoot;
    if (root->numofkeys > 0)
        return root;

    if (!root->isLeaf) {
        getNodeFromPage(BTreeNode, &newRoot, getPageNum(root->pointer[0]));
        newRoot->parent = 0;
        newRoot->pageNum = 2;
        writeNodeToPage(BTreeNode, newRoot);
    } else {
        newRoot = NULL;
    }
    free(root->keys);
    free(root->pointer);
    free(root);
    printf("ModifyRoot End\n");
    return newRoot;
}

/**
 * Merge Nodes with neighbours after deletion
 * @param BTreeNode
 * @param nodee
 * @param neighbor
 * @param neighbor_index
 * @param k_prime
 * @return updated root
 */
TreeNode *mergeNodes(BTreeManager *BTreeNode, TreeNode *nodee, TreeNode *neighbor, int neighbor_index, Value *k_prime) {
    printf("mergeNodes Start\n");
    int i, j, insertionIndex, nEnd;
    TreeNode *tmp;
    int order = BTreeNode->order;
    if (neighbor_index == -1) {
        tmp = nodee;
        nodee = neighbor;
        neighbor = tmp;
    }
    insertionIndex = neighbor->numofkeys;
    if (!nodee->isLeaf) {
        neighbor->keys[insertionIndex] = k_prime;
        neighbor->numofkeys++;
        nEnd = nodee->numofkeys;
        for (i = insertionIndex + 1, j = 0; j < nEnd; i++, j++) {
            neighbor->keys[i] = nodee->keys[j];
            neighbor->pointer[i] = nodee->pointer[j];
            neighbor->numofkeys++;
            nodee->numofkeys--;
        }
        neighbor->pointer[i] = nodee->pointer[j];
        for (i = 0; i < neighbor->numofkeys + 1; i++) {
            getNodeFromPage(BTreeNode, &tmp, getPageNum(neighbor->pointer[i]));
            tmp->parent = neighbor->pageNum;
            writeNodeToPage(BTreeNode, tmp);
        }
    } else {
        for (i = insertionIndex, j = 0; j < nodee->numofkeys; i++, j++) {
            neighbor->keys[i] = nodee->keys[j];
            neighbor->pointer[i] = nodee->pointer[j];
            neighbor->numofkeys++;
        }
        neighbor->pointer[order - 1] = nodee->pointer[order - 1];
    }
    writeNodeToPage(BTreeNode, neighbor);
    getNodeFromPage(BTreeNode, &tmp, nodee->parent);

    BTreeNode->root = DeleteEntry(BTreeNode, tmp, k_prime, &nodee->pageNum);
    free(nodee->keys);
    free(nodee->pointer);
    free(nodee);
    printf("mergeNodes End\n");
    return BTreeNode->root;
}

/**
 * Delete an entry and maintain B+ tree properties
 * @param BTreeNode
 * @param nodee
 * @param key
 * @param pointer
 * @return
 */
TreeNode *DeleteEntry(BTreeManager *BTreeNode, TreeNode *nodee, Value *key, void * pointer) {
    printf("DeleteEntry Begin\n");
    int minKeys;
    TreeNode *neighbor;
    int neighborIndex;
    int kPrimeIndex;
    Value *k_prime;
    int capacity;
    int order = BTreeNode->order;

    nodee = RemoveEntryOfNode(BTreeNode, nodee, key, pointer);
    if (nodee == BTreeNode->root)
        return ModifyRoot(BTreeNode, BTreeNode->root);

    TreeNode *nodeeParent;
    getNodeFromPage(BTreeNode, &nodeeParent, nodee->parent);

    if (nodee->isLeaf) {
        if ((order - 1) % 2 == 0)
            minKeys = (order - 1) / 2;
        else
            minKeys = (order - 1) / 2 + 1;
    } else {
        if ((order) % 2 == 0)
            minKeys = (order) / 2;
        else
            minKeys = (order) / 2 + 1;
        minKeys--;
    }

    if (nodee->numofkeys >= minKeys)
        return BTreeNode->root;
    neighborIndex = getNeighborIndex(BTreeNode, nodee);
    kPrimeIndex = neighborIndex == -1 ? 0 : neighborIndex;
    k_prime = nodeeParent->keys[kPrimeIndex];
    getNodeFromPage(BTreeNode, &neighbor,
            (neighborIndex == -1) ? getPageNum(nodeeParent->pointer[1]) : getPageNum(nodeeParent->pointer[neighborIndex]));

    capacity = nodee->isLeaf ? order : order - 1;

    printf("DeleteEntry End\n");
    if (neighbor->numofkeys + nodee->numofkeys < capacity)
        return mergeNodes(BTreeNode, nodee, neighbor, neighborIndex, k_prime);
    else
        return ReBalanceNodes(BTreeNode, BTreeNode->root, nodee, neighbor, neighborIndex, kPrimeIndex, k_prime);
}
/**
 * Delete record having the specified key
 * @param BTreeNode
 * @param key
 * @return Root of tree with Deleted key
 */
TreeNode *DeleteNode(BTreeManager *BTreeNode, Value *key) {
    printf("DeleteNode Begin\n");
    NodeData *record = searchRecord(BTreeNode, BTreeNode->root, key);
    TreeNode *LeafKey = searchLeaf(BTreeNode, BTreeNode->root, key);
    if (record != NULL && LeafKey != NULL) {
        BTreeNode->root = DeleteEntry(BTreeNode, LeafKey, key, record);
        free(record);
    }
    BTreeNode->numNodes--;
    printf("DeleteNode End\n");
    return BTreeNode->root;
}

/**
 * redistributes the entries between two nodes when one has become too small after deletion
 * but its neighbor is too big to append the small node's entries without exceeding the maximum
 * @param root
 * @param n
 * @param neighbor
 * @param neighbor_index
 * @param k_prime_index
 * @param k_prime
 * @return root of reblanced tree
 */
TreeNode *
ReBalanceNodes(BTreeManager *BTreeNode, TreeNode *root, TreeNode *n, TreeNode *neighbor, int neighbor_index, int k_prime_index, Value *k_prime) {
    printf("ReBalanceNodes Begin\n");
    int i;
    TreeNode *tmp;
    TreeNode *nParent;
    getNodeFromPage(BTreeNode, &nParent, n->parent);

    if (neighbor_index != -1) {
        if (!n->isLeaf)
            n->pointer[n->numofkeys + 1] = n->pointer[n->numofkeys];
        for (i = n->numofkeys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointer[i] = n->pointer[i - 1];
        }
        if (!n->isLeaf) {
            n->pointer[0] = neighbor->pointer[neighbor->numofkeys];
            getNodeFromPage(BTreeNode, &tmp, getPageNum(n->pointer[0]));
            tmp->parent = n->pageNum;
            neighbor->pointer[neighbor->numofkeys] = 0;
            n->keys[0] = k_prime;
            nParent->keys[k_prime_index] = neighbor->keys[neighbor->numofkeys - 1];
            writeNodeToPage(BTreeNode, tmp);
        } else {
            n->pointer[0] = neighbor->pointer[neighbor->numofkeys - 1];
            neighbor->pointer[neighbor->numofkeys - 1] = 0;
            n->keys[0] = neighbor->keys[neighbor->numofkeys - 1];
            nParent->keys[k_prime_index] = n->keys[0];
        }
    } else {
        if (n->isLeaf) {
            n->keys[n->numofkeys] = neighbor->keys[0];
            n->pointer[n->numofkeys] = neighbor->pointer[0];
            nParent->keys[k_prime_index] = neighbor->keys[1];
        } else {
            n->keys[n->numofkeys] = k_prime;
            n->pointer[n->numofkeys + 1] = neighbor->pointer[0];
            getNodeFromPage(BTreeNode, &tmp, getPageNum(n->pointer[n->numofkeys + 1]));
            tmp->parent = n->pageNum;
            nParent->keys[k_prime_index] = neighbor->keys[0];
            writeNodeToPage(BTreeNode, tmp);
        }
        for (i = 0; i < neighbor->numofkeys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointer[i] = neighbor->pointer[i + 1];
        }
        if (!n->isLeaf)
            neighbor->pointer[i] = neighbor->pointer[i + 1];
    }
    n->numofkeys++;
    neighbor->numofkeys--;

    writeNodeToPage(BTreeNode, neighbor);
    writeNodeToPage(BTreeNode, n);
    writeNodeToPage(BTreeNode, nParent);

    printf("ReBalanceNodes End\n");
    return root;
}
/**
 * Add nodes to queue to print the B+ tree
 * @param BTreeNode
 * @param newnode
 */
void enqueue(BTreeManager *BTreeNode, TreeNode *newnode) {
    TreeNode *c;
    if (BTreeNode->queue == NULL) {
        BTreeNode->queue = newnode;
        BTreeNode->queue->next = 0;
    } else {
        c = BTreeNode->queue;
        while (c->next != 0) {
            getNodeFromPage(BTreeNode, &c, c->next);
        }
        getNodeFromPage(BTreeNode, &c, newnode->pageNum);
        newnode->next = 0;
    }
}
/**
 * Access elements on queue
 * @param BTreeNode
 * @return next queue Item
 */
TreeNode *dequeue(BTreeManager *BTreeNode) {
    TreeNode *n = BTreeNode->queue;
    getNodeFromPage(BTreeNode, &BTreeNode->queue, BTreeNode->queue->next);
    n->next = 0;
    return n;
}

/**
 * Get the Length of Path to root
 * @param root
 * @param child
 * @return Length of path to root
 */
int RootPath(BTreeManager *BTreeNode, TreeNode *root, TreeNode *child) {
    int length = 0;
    TreeNode *c = child;
    while (c != root) {
        getNodeFromPage(BTreeNode, &c, c->parent);
        length++;
    }
    return length;
}
/**
 * Compare keys to find out which one is lesser
 * @param key1
 * @param key2
 * @return True if key1 is less than key2
 */
bool isLess(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            if (key1->v.intV < key2->v.intV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_FLOAT:
            if (key1->v.floatV < key2->v.floatV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_STRING:
            if (strcmp(key1->v.stringV, key2->v.stringV) == -1) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_BOOL:
            return FALSE;
            break;
    }
}

/**
 * Compare keys to find out which one is Greater
 *
 * @param key1
 * @param key2
 * @return TRUE if key1 is greater than key2
 */
bool isGreater(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            if (key1->v.intV > key2->v.intV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_FLOAT:
            if (key1->v.floatV > key2->v.floatV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_STRING:
            if (strcmp(key1->v.stringV, key2->v.stringV) == 1) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_BOOL:
            return FALSE;
            break;
    }
}

// This function compares two keys and returns TRUE if first key is equal to the second key else returns FALSE.
/**
 * Compare keys to find out if they are equall
 *
 * @param key1
 * @param key2
 * @return TRUE if key1 is equall to key2
 */
bool isEqual(Value *key1, Value *key2) {
    switch (key1->dt) {
        case DT_INT:
            if (key1->v.intV == key2->v.intV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_FLOAT:
            if (key1->v.floatV == key2->v.floatV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_STRING:
            if (strcmp(key1->v.stringV, key2->v.stringV) == 0) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
        case DT_BOOL:
            if (key1->v.boolV == key2->v.boolV) {
                return TRUE;
            } else {
                return FALSE;
            }
            break;
    }
}

int getPageNum(void * pointer) {
    if (pointer == NULL) return NULL;
    int tmp[4];
    memcpy(tmp, pointer, sizeof(int));
    return *(int *)tmp;
}


RC writeNodeToPage(BTreeManager *tree, TreeNode *node) {

    //Read the page with the node
    RC response;
    BM_PageHandle *nodePage = malloc(sizeof(BM_PageHandle));
    nodePage->data = malloc(PAGE_SIZE* sizeof(char));
    nodePage->pageNum = node->pageNum;
    response = pinPage(tree->bufferPool, nodePage, node->pageNum);
    if (response != RC_OK) return response;

    memcpy(nodePage->data, &(node->parent), sizeof(int));
    memcpy(nodePage->data + sizeof(int), &(node->numofkeys), sizeof(int));
    memcpy(nodePage->data + sizeof(int)*2, &(node->isLeaf), sizeof(bool));
    unsigned int offset = sizeof(int)*2+ sizeof(bool);
    for (int i = 0; i < node->numofkeys; i++){
        //Supports only int keys
        Value *val = malloc(sizeof(Value));
        val->dt = DT_INT;
        memcpy(val, node->keys[i], sizeof(Value));
        memcpy(nodePage->data + offset, &(val->v.intV), sizeof(int));
        offset += sizeof(int);
        //pointer is page num, 0 if null
        memcpy(nodePage->data + offset, node->pointer[i], sizeof(int));
        offset += sizeof(int);

        if (node->isLeaf){
            memcpy(nodePage->data + offset,node->pointer[i], sizeof(RID));
            offset += sizeof(RID);
        }
        else {
            memcpy(nodePage->data + offset,node->pointer[i], sizeof(int));
            offset += sizeof(int);
        }
    }

    memcpy(nodePage->data, &(node->next), sizeof(int));
    response = markDirty(tree->bufferPool,nodePage);
    if (response != RC_OK) return response;
    response = unpinPage(tree->bufferPool,nodePage);
    if (response != RC_OK) return response;

    //free(nodePage->data);
    free(nodePage);

    return response;


}

RC getNodeFromPage(BTreeManager *tree, TreeNode **node, int pageNum) {

    (*node) = createNode(tree);
    (*node)->isLeaf = true;
    (*node)->pageNum = pageNum;

    //Read the page with the node
    RC response;
    BM_PageHandle *nodePage = malloc(sizeof(BM_PageHandle));
    nodePage->data = malloc(PAGE_SIZE* sizeof(char));
    nodePage->pageNum = 1;
    response = pinPage(tree->bufferPool,nodePage,pageNum);
    if (response != RC_OK) return response;

    //Fill node
    int tmp[4];

    memcpy(tmp, nodePage->data, sizeof(int));
    (*node)->parent = *(int *)tmp;

    memcpy(tmp, nodePage->data + sizeof(int), sizeof(int));
    (*node)->numofkeys = *(int *)tmp;

    memcpy(tmp, nodePage->data + sizeof(int)*2, sizeof(bool));
    (*node)->isLeaf = *(bool *)tmp;

    unsigned int offset = sizeof(int)*2+ sizeof(bool);
    for (int i = 0; i < (*node)->numofkeys; i++){
        //Supports only int keys
        memcpy(tmp, nodePage->data + offset, sizeof(int));
        Value *val = malloc(sizeof(Value));
        val->dt = DT_INT;
        val->v.intV = *(int *)tmp;
        memcpy((*node)->keys + i*sizeof(Value), &val, sizeof(Value));
        //pointer is page num, 0 if null
        offset += sizeof(int);

        if ((*node)->isLeaf){
            RID *rec = malloc(sizeof(RID));
            memcpy(rec, nodePage->data + offset, sizeof(RID));
            memcpy((*node)->pointer + i*sizeof(RID), rec, sizeof(RID));
            offset += sizeof(RID);
        }
        else {
            memcpy(tmp, nodePage->data + offset, sizeof(int));
            void * pointer = (int *)tmp;
            offset += sizeof(int);
            memcpy((*node)->pointer + i*sizeof(int), &pointer, sizeof(int));
        }

    }

    memcpy(tmp, nodePage->data + offset, sizeof(int));
    (*node)->next = *(int *)tmp;

    response = unpinPage(tree->bufferPool,nodePage);

    return response;
}

RC freeNode(TreeNode *node) {
    for (int i = 0; i < node->numofkeys; i++) {
        free(node->pointer + i * sizeof(int));
        free(node->keys + i * sizeof(int));
    }

    free(node);
}
