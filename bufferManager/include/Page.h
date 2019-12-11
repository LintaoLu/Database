#ifndef PAGE_H
#define PAGE_H
#include <stdbool.h>

typedef struct Page {
    //1 means dirty, 0 means clean
    bool isDirty;
    bool isPinned;
    int fixCount;
    int pageNumber;
    char *pageValue;
} Page;

#endif