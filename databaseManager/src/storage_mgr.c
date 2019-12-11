//
// Created by jaime on 8/24/19.
//
#include <stdio.h>
#include <storage_mgr.h>
#include <dberror.h>
#include <stdlib.h>
#include <string.h>

void initStorageManager() {

}

RC createPageFile(char *fileName) {

    FILE *file = fopen(fileName, "wb"); //I have change this to binary mode. JC

    if (file == NULL) return RC_FILE_NOT_FOUND;

    SM_PageHandle zeroedBlock = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));

    if (fwrite(zeroedBlock, sizeof(char), PAGE_SIZE, file) < PAGE_SIZE) {
        free(zeroedBlock);
        return RC_WRITE_FAILED;
    }

    if (fwrite(zeroedBlock, sizeof(char), PAGE_SIZE, file) < PAGE_SIZE) {
        free(zeroedBlock);
        return RC_WRITE_FAILED;
    }

    free(zeroedBlock);

    updateTotalPages(file, 1);

    fclose(file);

	return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *file = fopen(fileName, "rb+"); //I have change this to binary mode. JC

    if (file == NULL)
        return RC_FILE_NOT_FOUND;

    int pageTotal;
    readTotalPages(file, &pageTotal);

    fHandle->fileName = fileName;
    fHandle->totalNumPages = pageTotal;
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = file;

    return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle) {
    if (fHandle->fileName == NULL) return RC_FILE_HANDLE_NOT_INIT;

    if (fclose(fHandle->mgmtInfo) == 0) return RC_OK;

    return RC_FILE_NOT_FOUND;  //TODO: Add new error?
}

RC destroyPageFile(char *fileName) {
    if (remove(fileName) == 0) return RC_OK;

    return RC_FILE_NOT_FOUND;
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->mgmtInfo == NULL) return RC_FILE_HANDLE_NOT_INIT;

    if (pageNum > fHandle->totalNumPages || pageNum < 0) return RC_READ_NON_EXISTING_PAGE;

    FILE *file = (FILE *) fHandle->mgmtInfo;
    if (fseek(file, pageNum * PAGE_SIZE, SEEK_SET) == 0) {
        //if(fread(memPage, PAGE_SIZE, sizeof(char), file) < PAGE_SIZE) return RC_READ_NON_EXISTING_PAGE;
        fread(memPage, PAGE_SIZE, sizeof(char), file);
		return RC_OK;
	}
	return RC_FILE_NOT_FOUND;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(1, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages, fHandle, memPage);
}

RC getBlockPos(SM_FileHandle *fHandle){ return fHandle->curPagePos; }

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (pageNum < 0) return RC_WRITE_FAILED;

    if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;

    FILE *file = fHandle->mgmtInfo;

    if (fseek(file, (pageNum * PAGE_SIZE), SEEK_SET) == 0) {
        if (fwrite(memPage, sizeof(char), PAGE_SIZE, file) < PAGE_SIZE) return RC_WRITE_FAILED;
        fHandle->curPagePos = (int) ftell(file) / PAGE_SIZE;
        return RC_OK;
    }
    return RC_WRITE_FAILED;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
    //Calloc: The effective result is the allocation of a zero-initialized memory block of (num*size) bytes.
    SM_PageHandle zeroedBlock = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));
    if (writeBlock(fHandle->totalNumPages + 1, fHandle, zeroedBlock) != RC_OK) {
        free(zeroedBlock);
        return RC_WRITE_FAILED;
    }
    free(zeroedBlock);
    fHandle->totalNumPages++;

    updateTotalPages(fHandle->mgmtInfo, fHandle->totalNumPages);

    return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    RC status = RC_OK;
    //We are not counting the header page. If you wnat three pages, it means three + header
    while (numberOfPages > fHandle->totalNumPages) {
        status = appendEmptyBlock(fHandle);
        if (status != RC_OK) break;
    }
    return status;
}

RC updateTotalPages(FILE *file, int newTotalPages) {
    if (fseek(file, 0, SEEK_SET) == 0) {
        if (fwrite(&newTotalPages, sizeof(int), 1, file) < PAGE_SIZE) return RC_OK;
        if (fseek(file, newTotalPages * PAGE_SIZE, SEEK_SET) == 0) return RC_OK;
        return RC_WRITE_FAILED;
    }
    return RC_WRITE_FAILED;
}

RC readTotalPages(FILE *file, int *pageTotal) {
    if (fread(pageTotal, sizeof(int), 1, file) < PAGE_SIZE) return RC_WRITE_FAILED;
    return RC_OK;
}
