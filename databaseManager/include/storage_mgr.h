#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <dberror.h>

/**
 * @brief Holds the metadata for the page file.
 *
 */
typedef struct SM_FileHandle {
	char *fileName; /**< Name of the page file. */
	int totalNumPages; /**< Maximum number of pages allotted to this page file */
	int curPagePos; /**< The current position of the file. Who makes this an int?*/
	void *mgmtInfo; /**< No idea yet. */
} SM_FileHandle;

typedef char* SM_PageHandle;

/************************************************************
 *                    interface                             *
 ************************************************************/
/* manipulating page files */
extern void initStorageManager(void);
extern RC createPageFile(char *fileName);
extern RC openPageFile(char *fileName, SM_FileHandle *fHandle);
extern RC closePageFile(SM_FileHandle *fHandle);
extern RC destroyPageFile(char *fileName);

/* reading blocks from disc */
extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern int getBlockPos(SM_FileHandle *fHandle);
extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);

/* writing blocks to a page file */
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle,
		SM_PageHandle memPage);
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock(SM_FileHandle *fHandle);
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle);

extern RC updateTotalPages(FILE *file, int newTotalPages);

RC readTotalPages(FILE *file, int *pageTotal);

#endif
