//
// Created by Marcin Matuk on 10/27/19.
//
// Use this file to create all custome structs needed for the project

#ifndef DATABASE_CUSTOM_STRUCTS_H
#define DATABASE_CUSTOM_STRUCTS_H

#include <buffer_mgr.h>

typedef struct RM_MgmtData {
    BM_BufferPool *bufferPool;
    int tupleCount;
    int *tupleCountPerPage;
    bool dirty;
    int numPages;
} RM_MgmtData;

typedef struct RM_scanMgmtData {
    RID current_record;
    Expr *condition;
    int record_count;
} RM_scanMgmtData;

#endif //DATABASE_CUSTOM_STRUCTS_H
