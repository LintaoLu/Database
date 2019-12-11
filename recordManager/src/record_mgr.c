//
// Created by Marcin Matuk on 10/27/19.
//

#include <record_mgr.h>
#include <custom_structs.h>
#include <dberror.h>
//#include <stdint-gcc.h>


char* createFilename(char *tableName);
char* schemaToChar(Schema *schema);
Schema* charToSchema(char *schema_ser);
unsigned int getRecordAttributeOffset(Schema *schema, int attrNum);

int getSchemaSize(Schema *schema);

// table and manager
RC initRecordManager(void *mgmtData) {
    return RC_OK;
}

RC shutdownRecordManager(){
    return RC_OK;
}

RC createTable(char *name, Schema *schema) {
    //Append '.bin' to table name to create filename
    char *filename = createFilename(name);

    //Use storage manager to create a new file for the table
    RC response;
    response = createPageFile(filename);
    if (response != RC_OK) return response;


    //Init buffer pool to create new pages
    BM_BufferPool *bufferPool = (BM_BufferPool*) malloc(sizeof(BM_BufferPool));
    initBufferPool(bufferPool,filename,5,RS_LRU,NULL);

    //Serialize schema and write to page 1 of file
    BM_PageHandle *schemaPage = malloc(sizeof(BM_PageHandle));
    schemaPage->data = malloc(PAGE_SIZE* sizeof(char));
    schemaPage->pageNum = 1;

    response = pinPage(bufferPool, schemaPage,1);
    if (response != RC_OK) return response;

    //Serialize schema and write to page 1 of file
    char *schema_ser = schemaToChar(schema);
    int recordSize = getSchemaSize(schema);
    memcpy(schemaPage->data,schema_ser,recordSize);

    markDirty(bufferPool, schemaPage);
    response = unpinPage(bufferPool, schemaPage);
    if (response != RC_OK) return response;

    //Write page 2 to file with tuple count int and array for tuple count per page
    BM_PageHandle *memMgmtPage = malloc(sizeof(BM_PageHandle));
    memMgmtPage->data = malloc(PAGE_SIZE* sizeof(char));
    memMgmtPage->pageNum = 2;
    response = pinPage(bufferPool, memMgmtPage,2);
    if (response != RC_OK) return response;

    //response = updateTotalPages(fHandle->mgmtInfo,2); //TODO FIX updateTotalPages

    //markDirty(bufferPool, memMgmtPage);
    response = unpinPage(bufferPool, memMgmtPage);
    if (response != RC_OK) return response;

    BM_PageHandle *firstTuplePage = malloc(sizeof(BM_PageHandle));
    firstTuplePage->data = malloc(PAGE_SIZE* sizeof(char));
    firstTuplePage->pageNum = 3;
    response = pinPage(bufferPool, firstTuplePage,3);
    if (response != RC_OK) return response;

    markDirty(bufferPool, firstTuplePage);
    response = unpinPage(bufferPool, firstTuplePage);
    if (response != RC_OK) return response;

    //Free all memory
    free(filename);
    free(memMgmtPage);
    free(schemaPage);
    response = shutdownBufferPool(bufferPool);

    return response;

}

RC openTable(RM_TableData *rel, char *name) {
    // Get file name based on table name
    char *filename = createFilename(name);

    rel->name = name;
    //Create buffer pool and storage manager and add those to the mgmt info
    RM_MgmtData *mgmtData = (RM_MgmtData*) malloc(sizeof(RM_MgmtData));
    rel->mgmtData = mgmtData;
    BM_BufferPool *bufferPool = (BM_BufferPool*) malloc(sizeof(BM_BufferPool));
    initBufferPool(bufferPool,filename,5,RS_LRU,NULL); //TODO Set num pages to some config
    mgmtData->bufferPool = bufferPool;
    mgmtData->dirty = false;
    BM_Mgmt_Info *bi = bufferPool->mgmtData;
    SM_FileHandle *fh = bi->fHandle;
    mgmtData->numPages = fh->totalNumPages;

    RC response;

    //Get schema from file
    BM_PageHandle *schemaPage = malloc(sizeof(BM_PageHandle));
    schemaPage->data = malloc(PAGE_SIZE* sizeof(char));
    schemaPage->pageNum = 1;
    response = pinPage(bufferPool,schemaPage,1);
    if (response != RC_OK) return response;

    Schema *schema = charToSchema(schemaPage->data);
    rel->schema = schema;

    response = unpinPage(bufferPool,schemaPage);
    if (response != RC_OK) return response;

    //Load 2nd page with tuple counts and populate the mgmtinfo. Add to buffer

    if (mgmtData->numPages < 3) {
        mgmtData->tupleCount = 0;
        mgmtData->tupleCountPerPage = NULL;
    }
    else {
        BM_PageHandle *memMgmtPage = malloc(sizeof(BM_PageHandle));
        memMgmtPage->data = malloc(PAGE_SIZE* sizeof(char));
        memMgmtPage->pageNum = 2;
        response = pinPage(bufferPool,memMgmtPage,2);
        if (response != RC_OK) return response;
        char tmp[sizeof(int)];
        memcpy(tmp,memMgmtPage->data, sizeof(int));
        mgmtData->tupleCount = *(uint32_t *)tmp;

        int* tupleCountbyPage = malloc(sizeof(int)*(mgmtData->numPages-2));
        for (int i = 0;i<mgmtData->numPages-2;i++){
            memcpy(tmp,memMgmtPage->data + i*sizeof(int), sizeof(int));
            tupleCountbyPage[i] = *(uint32_t *)tmp;
        }
        mgmtData->tupleCountPerPage = tupleCountbyPage;
        response = unpinPage(bufferPool,memMgmtPage);
        if (response != RC_OK) return response;
        free(memMgmtPage);
    }

    free(schemaPage);

    return RC_OK;
}

RC closeTable(RM_TableData *rel){
    RM_MgmtData *mgmtData = rel->mgmtData;
    BM_BufferPool *bufferPool = mgmtData->bufferPool;
    //TODO When inserting/deleting records, we must update mgmtinfo with the new counts. Will the page be updated then or just now?
    //updateTotalPages(mgmtData->storageManager->mgmtInfo,mgmtData->storageManager->totalNumPages);

    if (mgmtData->dirty) {
        BM_PageHandle *const pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
        pageHandle->pageNum = 2;
        pageHandle->data = malloc(PAGE_SIZE* sizeof(char));
        pinPage(bufferPool,pageHandle,2);
        memcpy(pageHandle->data,&mgmtData->tupleCount,sizeof(int));
        memcpy(pageHandle->data + sizeof(int),&mgmtData->tupleCountPerPage,sizeof(int)*mgmtData->numPages);
        markDirty(bufferPool,pageHandle);
    }

    unPinAll(bufferPool);
    RC response;
    if ((response = shutdownBufferPool(bufferPool)) != RC_OK) return response;

    if (mgmtData->tupleCountPerPage) free(mgmtData->tupleCountPerPage);
    if(mgmtData) free(mgmtData);
    if(rel->schema) free(rel->schema);
    //if(rel->name) free(rel->name);

    return RC_OK;
}

RC deleteTable(char *name){
    char *filename = createFilename(name);
    return destroyPageFile(filename);
}

int getNumTuples(RM_TableData *rel){
    RM_MgmtData *mgmtData = rel->mgmtData;
    return mgmtData->tupleCount;
}

// handling records in a table
RC insertRecord(RM_TableData *rel, Record *record){

    if(rel == NULL) return RC_FILE_NOT_FOUND;
    unsigned int recordSize = getRecordSize(rel->schema);

    RM_MgmtData *mgmtData = rel->mgmtData; //custom struct
    int* array = mgmtData->tupleCountPerPage; //this array tells us how many records are saved in each page

    unsigned int maxTupleNum = (PAGE_SIZE - PAGE_HEADER_SIZE) / (recordSize + TUPLE_HEADER_SIZE);

    int numOfPages = (mgmtData->numPages - 2); //page size = number of pages
    int pageNum = numOfPages;


    for(int i = 0; i < numOfPages; i++) { // page 0~2 save metadata, not records
        if (array[i] < maxTupleNum) {
            pageNum = i;
            break;
        }
    }

    if(pageNum == numOfPages)
    {
        mgmtData->numPages++;
        int * tmp = mgmtData->tupleCountPerPage;
        int* tupleCountbyPage = calloc(sizeof(int), (numOfPages + 1));
        memcpy(tupleCountbyPage,tmp,sizeof(int)*(numOfPages));
        pageNum = numOfPages;
        free(tmp);
        mgmtData->tupleCountPerPage = tupleCountbyPage;
    }

    BM_BufferPool *bufferPool = mgmtData->bufferPool;
    BM_PageHandle *const pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    pageHandle->pageNum = (pageNum+3);
    pageHandle->data = malloc(PAGE_SIZE* sizeof(char));
    pinPage(bufferPool, pageHandle, pageNum+3);

    record->id.page = (pageNum + 3);

    int id = 0;
    unsigned int offset = PAGE_HEADER_SIZE;
    char tmp[sizeof(int)];

    while (id < (int)maxTupleNum) {

        memcpy(tmp,pageHandle->data + offset,sizeof(int));
        int isFree = *(int *)tmp;
        if (isFree == 0) {
            int slot = (id + 1);
            memcpy(pageHandle->data + offset, &(slot), sizeof(int));
            memcpy(pageHandle->data + offset + TUPLE_HEADER_SIZE,record->data,recordSize);
            record->id.slot = slot;
            break;
        }
        else{
            id++;
            offset += (TUPLE_HEADER_SIZE + recordSize);
        }
    }
    mgmtData->dirty = true;
    markDirty(bufferPool,pageHandle); //TODO Error handling
    unpinPage(bufferPool, pageHandle);

    mgmtData->tupleCount++;
    mgmtData->tupleCountPerPage[pageNum]++;

    return RC_OK;
}

RC deleteRecord(RM_TableData *rel, RID id){

    RM_MgmtData *mgmtData = rel->mgmtData;

    BM_BufferPool *bufferPool = mgmtData->bufferPool;
    BM_PageHandle *const pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    pageHandle->pageNum = id.page;
    pageHandle->data = malloc(PAGE_SIZE* sizeof(char));
    pinPage(bufferPool, pageHandle, pageHandle->pageNum);

    unsigned int recordSize = getRecordSize(rel->schema);

    unsigned int offset = PAGE_HEADER_SIZE + (id.slot-1) * (recordSize + TUPLE_HEADER_SIZE);

    int off = 0;
    memcpy(pageHandle->data + offset,&off,sizeof(int));
    mgmtData->dirty = true;
    markDirty(bufferPool,pageHandle); //TODO Error handling
    unpinPage(bufferPool, pageHandle);

    mgmtData->tupleCount--;
    int* array = mgmtData->tupleCountPerPage;
    int newCount = array[id.page-2] - 1;
    memcpy(mgmtData->tupleCountPerPage + (id.page-2)* sizeof(int),&newCount, sizeof(int));

    return RC_OK;
}

RC updateRecord(RM_TableData *rel, Record *record){

    RM_MgmtData *mgmtData = rel->mgmtData;

    BM_BufferPool *bufferPool = mgmtData->bufferPool;
    BM_PageHandle *const pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    pageHandle->pageNum = record->id.page;
    pageHandle->data = malloc(PAGE_SIZE* sizeof(char));
    pinPage(bufferPool, pageHandle, record->id.page);

    unsigned int recordSize = getRecordSize(rel->schema);

    unsigned int offset = PAGE_HEADER_SIZE + (record->id.slot-1) * (recordSize + TUPLE_HEADER_SIZE) + TUPLE_HEADER_SIZE;
    memcpy(pageHandle->data + offset,record->data,recordSize);
    markDirty(bufferPool,pageHandle); //TODO Error handling
    unpinPage(bufferPool, pageHandle);

    return RC_OK;
}

RC getRecord(RM_TableData *rel, RID id, Record *record){

    //createRecord(&record,rel->schema);
    record->id.page = id.page;
    record->id.slot = id.slot;

    RM_MgmtData *mgmtData = rel->mgmtData;
    BM_BufferPool *bufferPool = mgmtData->bufferPool;
    BM_PageHandle *const pageHandle = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    pageHandle->pageNum = record->id.page;
    pageHandle->data = malloc(PAGE_SIZE* sizeof(char));
    pinPage(bufferPool, pageHandle, record->id.page);
    //readBlock(3,mgmtData->storageManager,pageHandle->data);
    unsigned int recordSize = getRecordSize(rel->schema);

    unsigned int offset = PAGE_HEADER_SIZE + (record->id.slot - 1) * (recordSize + TUPLE_HEADER_SIZE);

    char tmp [sizeof(int)];


    memcpy(tmp,pageHandle->data + offset,sizeof(int));
    int th = *(int*) tmp;

    if (th == 0) {
        return RC_RM_NO_MORE_TUPLES;
    }
    offset += TUPLE_HEADER_SIZE;
    memcpy(record->data,pageHandle->data + offset,recordSize);
    unpinPage(bufferPool, pageHandle);

    return RC_OK;
}

// scans
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){
    if (cond == NULL) {
        return RC_SCAN_COND_UNKOWN;
    }

    RM_scanMgmtData *scan_MgmtData;

    scan_MgmtData = (RM_scanMgmtData *) malloc(sizeof(RM_scanMgmtData));

    scan->mgmtData = scan_MgmtData;
    scan->rel = rel;

    scan_MgmtData->current_record.page = 3;    // Page 1 and 2 are for metadata TODO: check
    scan_MgmtData->current_record.slot = 1;
    scan_MgmtData->record_count = 0;
    scan_MgmtData->condition = cond;

    return RC_OK;
}

RC next(RM_ScanHandle *scan, Record *record){
    RM_scanMgmtData *scan_MgmtData = scan->mgmtData;
    Schema *schema = scan->rel->schema;

    Value *result = (Value *) malloc(sizeof(Value));
    RC getResult;

    int recordSize = getRecordSize(schema);  // Getting record size of the schema
    int slotsPerPage = (PAGE_SIZE - PAGE_HEADER_SIZE) / (recordSize + TUPLE_HEADER_SIZE);
    int slotsInTable = getNumTuples(scan->rel);

    if (slotsInTable == 0)
        return RC_RM_NO_MORE_TUPLES;

    // Iterate through all tuples
    while (scan_MgmtData->record_count < slotsInTable) {
        getResult = getRecord(scan->rel, scan_MgmtData->current_record, record);

        if (getResult == RC_OK) { //If it is null do not count it.
            scan_MgmtData->record_count++;

            // Test record
            evalExpr(record, schema, scan_MgmtData->condition, &result);

            scan_MgmtData->current_record.slot++;
            if (scan_MgmtData->current_record.slot > slotsPerPage) { //TODO: maybe >=
                scan_MgmtData->current_record.slot = 1;
                scan_MgmtData->current_record.page++;
            }

            if (result->v.boolV == TRUE) {
                return RC_OK;
            }


        }
        else{
            scan_MgmtData->current_record.slot++;
            if (scan_MgmtData->current_record.slot > slotsPerPage) { //TODO: maybe >=
                scan_MgmtData->current_record.slot = 1;
                scan_MgmtData->current_record.page++;
            }
        }
    }
    //There are no more tuples to scan
    return RC_RM_NO_MORE_TUPLES;

}

RC closeScan(RM_ScanHandle *scan) {
    RM_scanMgmtData *scan_MgmtData = scan->mgmtData;

    free(scan_MgmtData);

    return RC_OK;
}

// dealing with schemas
int getRecordSize(Schema *schema){
    return getRecordAttributeOffset(schema, schema->numAttr);
}

int getSchemaSize(Schema *schema) {
    int numAttr = schema->numAttr;
    unsigned int recordSize = sizeof(int)*2;

    unsigned int* attrNameLengths = malloc(sizeof(int)*numAttr);
    for(int i = 0;i < numAttr;i++) {
        recordSize += (unsigned)strlen(schema->attrNames[i])*sizeof(char);
        attrNameLengths[i] = (unsigned)strlen(schema->attrNames[i]);
    }

    recordSize += numAttr * (sizeof(DataType) + sizeof(int)*2) + sizeof(int)*schema->keySize;
    return recordSize;
}

Schema *
createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
    Schema *schema = malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;

    return schema;
}

RC freeSchema(Schema *schema){
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);
    free(schema);
    return RC_OK;
}

// dealing with records and attribute values
RC createRecord(Record **record, Schema *schema){
    *record = malloc(sizeof(Record));
    unsigned int recordLen = getRecordAttributeOffset(schema,schema->numAttr);

    (*record)->data = malloc(sizeof(char)*recordLen);

    return RC_OK;
}

RC freeRecord(Record *record) {
    free(record->data);
    free(record);
    return RC_OK;
}

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value){

    if (attrNum > schema->numAttr) {
        return RC_RM_NO_MORE_TUPLES;
    }

    unsigned int offset = getRecordAttributeOffset(schema,attrNum ); //assumes attrNum starts at 0
    *value = malloc(sizeof(Value));
    (*value)->dt = schema->dataTypes[attrNum];

    switch (schema->dataTypes[attrNum]) {
        case DT_INT: {
            char tmp[sizeof(int)];
            memcpy(tmp,record->data + offset,sizeof(int));
            (*value)->v.intV = *(int32_t *)tmp;
        }
            break;
        case DT_BOOL: {
            char tmp[sizeof(bool)];
            memcpy(tmp,record->data + offset,sizeof(bool));
            (*value)->v.boolV = *(bool *)tmp;
        }
            break;
        case DT_FLOAT: {
            char tmp[sizeof(float)];
            memcpy(tmp,record->data + offset,sizeof(float));
            (*value)->v.boolV = *(float *)tmp;
        }
            break;
        case DT_STRING: {
            (*value)->v.stringV = malloc(sizeof(char)*schema->typeLength[attrNum]);
            memcpy((*value)->v.stringV,record->data + offset,sizeof(char)*schema->typeLength[attrNum]);
        }
            break;
        default:
            return 0;
    }

    return RC_OK;
}

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value){

    if (attrNum > schema->numAttr) {
        return RC_RM_NO_MORE_TUPLES;
    }

    char* recData = record->data;

    unsigned int offset = getRecordAttributeOffset(schema,attrNum); //assumes attrNum starts at 0

    switch (schema->dataTypes[attrNum]) {
        case DT_INT: {
            memcpy(recData + offset,&value->v.intV,sizeof(int));
        }
            break;
        case DT_BOOL: {
            memcpy(recData + offset,&value->v.boolV,sizeof(bool));
        }
            break;
        case DT_FLOAT: {
            memcpy(recData + offset,&value->v.floatV,sizeof(float));
        }
            break;
        case DT_STRING: {
            memcpy((recData + offset),&(*value->v.stringV),sizeof(char)*schema->typeLength[attrNum]);
        }
            break;
        default:
            return 0;
    }

    return RC_OK;
}

char* createFilename(char *tableName) {
    char* filename = malloc(sizeof(char)*strlen(tableName)+sizeof(char)*4+1);
    strcpy(filename,tableName);
    strcat(filename,".bin");
    return filename;
}

char* schemaToChar(Schema *schema) {
    int numAttr = schema->numAttr;
    unsigned int recordSize = sizeof(int)*2;

    unsigned int* attrNameLengths = malloc(sizeof(int)*numAttr);
    for(int i = 0;i < numAttr;i++) {
        recordSize += (unsigned)strlen(schema->attrNames[i])*sizeof(char);
        attrNameLengths[i] = (unsigned)strlen(schema->attrNames[i]);
    }

    recordSize += numAttr * (sizeof(DataType) + sizeof(int)*2) + sizeof(int)*schema->keySize;

    char* schemaChar = malloc(recordSize);

    memcpy(schemaChar,&numAttr,sizeof(int));
    memcpy(schemaChar+sizeof(int),&(schema->keySize),sizeof(int));

    unsigned int offset = sizeof(int)*2;
    for (int i = 0; i<numAttr;i++) {
        memcpy(schemaChar + offset,&attrNameLengths[i], sizeof(int)); //attribute name length
        offset += sizeof(int);
        memcpy(schemaChar + offset,schema->attrNames[i], sizeof(char)*attrNameLengths[i]); //attribute name
        offset += sizeof(char)*attrNameLengths[i];
        memcpy(schemaChar + offset,&schema->dataTypes[i],sizeof(DataType)); //data type
        offset += sizeof(DataType);
        memcpy(schemaChar + offset,&schema->typeLength[i],sizeof(int)); //type length
        offset += sizeof(int);
    }

    memcpy(schemaChar + offset,schema->keyAttrs,sizeof(int)*schema->keySize); //key attribute

    free(attrNameLengths);

    return schemaChar;
}

Schema* charToSchema(char *schema_ser) {
    unsigned int offset = 0;
    char tmp[4];
    memcpy(tmp,schema_ser, sizeof(int));
    offset += sizeof(int);
    int numAttr = *(uint32_t *)tmp;
    memcpy(tmp,schema_ser + offset, sizeof(int));
    offset += sizeof(int);
    int keySize = *(uint32_t *)tmp;

    char **attrNames = malloc(numAttr* sizeof(char*));
    DataType *dataTypes = malloc(numAttr* sizeof(DataType));
    int *typeLength = malloc(numAttr* sizeof(int));
    int *keyAttrs = malloc(numAttr* sizeof(int));

    for (int i = 0; i<numAttr;i++) {
        memcpy(tmp,schema_ser + offset, sizeof(int)); //attribute name length
        int attrNameLen = *(uint32_t *)tmp;
        offset += sizeof(int);
        char *attrName = malloc(sizeof(char)*attrNameLen);
        memcpy(attrName,schema_ser + offset, sizeof(char)*attrNameLen); //attribute name
        attrNames[i] = attrName;
        offset += sizeof(char)*attrNameLen;
        memcpy(tmp,schema_ser + offset, sizeof(DataType)); //data type
        dataTypes[i] = (DataType)(*(uint32_t *)tmp);
        offset += sizeof(DataType);
        memcpy(tmp,schema_ser + offset, sizeof(int)); //type length
        typeLength[i] = *(uint32_t *)tmp;
        offset += sizeof(int);
    }

    for (int i = 0; i < keySize; i++) {
        memcpy(tmp,schema_ser + offset, sizeof(int));
        keyAttrs[i] = *(uint32_t *)tmp;
        offset += sizeof(int);
    }

    return createSchema(numAttr,attrNames,dataTypes,typeLength,keySize,keyAttrs);

}

unsigned int getRecordAttributeOffset(Schema *schema, int attrNum) {
    unsigned int offset = 0;

    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT: {
                offset += sizeof(int);
            }
                break;
            case DT_BOOL: {
                offset += sizeof(bool);
            }
                break;
            case DT_FLOAT: {
                offset += sizeof(float);
            }
                break;
            case DT_STRING: {
                offset += sizeof(char)*schema->typeLength[i];
            }
                break;
            default:
                return 0;
        }
    }

    return offset;
}
