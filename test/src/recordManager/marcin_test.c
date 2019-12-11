//
// Created by Marcin Matuk on 10/27/19.
//

#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"


#define ASSERT_EQUALS_RECORDS(_l, _r, schema, message)            \
        do {                                    \
            Record *_lR = _l;                                                   \
            Record *_rR = _r;                                                   \
            ASSERT_TRUE(memcmp(_lR->data,_rR->data,getRecordSize(schema)) == 0, message); \
            int i;                                \
            for(i = 0; i < schema->numAttr; i++)                \
            {                                    \
                Value *lVal, *rVal;                                             \
                char *lSer, *rSer; \
                getAttr(_lR, schema, i, &lVal);                                  \
                getAttr(_rR, schema, i, &rVal);                                  \
                lSer = serializeValue(lVal); \
                rSer = serializeValue(rVal); \
                ASSERT_EQUALS_STRING(lSer, rSer, "attr same");    \
                free(lVal); \
                free(rVal); \
                free(lSer); \
                free(rSer); \
            }                                    \
        } while(0)

#define ASSERT_EQUALS_RECORD_IN(_l, _r, rSize, schema, message)        \
        do {                                    \
            int i;                                \
            boolean found = false;                        \
            for(i = 0; i < rSize; i++)                        \
            if (memcmp(_l->data,_r[i]->data,getRecordSize(schema)) == 0)    \
            found = true;                            \
            ASSERT_TRUE(0, message);                        \
        } while(0)

#define OP_TRUE(left, right, op, message)        \
        do {                            \
            Value *result = (Value *) malloc(sizeof(Value));    \
            op(left, right, result);                \
            bool b = result->v.boolV;                \
            free(result);                    \
            ASSERT_TRUE(b,message);                \
        } while (0)

// test methods
static void testCreateTable(void);

static void testRecords(void);

// struct for test records
typedef struct TestRecord {
    int a;
    char *b;
    int c;
} TestRecord;

// helper methods
Record *testRecord(Schema *schema, int a, char *b, int c);

Schema *testSchema(void);

Record *fromTestRecord(Schema *schema, TestRecord in);

// test name
char *testName;

// main method
int
main(void) {
    testName = "";
    testCreateTable();
    testRecords();


    return 0;
}

// ************************************************************
void
testCreateTable(void) {
    RM_TableData *table = (RM_TableData *) malloc(sizeof(RM_TableData));

    Schema *schema;
    testName = "test creating a new table and inserting tuples";
    schema = testSchema();

    TEST_CHECK(initRecordManager(NULL));
    TEST_CHECK(createTable("test_table_r", schema));
    TEST_CHECK(openTable(table, "test_table_r"));
    TEST_CHECK(closeTable(table));
    TEST_CHECK(deleteTable("test_table_r"));
    TEST_CHECK(shutdownRecordManager());
    free(table);
    TEST_DONE();
}

void
testRecords(void) {
    TestRecord expected[] = {
            {1, "aaaa", 3},
    };
    Schema *schema;
    Record *r;
    Value *value;
    testName = "test creating records and manipulating attributes";

    // check attributes of created record
    schema = testSchema();
    r = fromTestRecord(schema, expected[0]);

    getAttr(r, schema, 0, &value);
    OP_TRUE(stringToValue("i1"), value, valueEquals, "first attr");
    freeVal(value);

    getAttr(r, schema, 1, &value);
    OP_TRUE(stringToValue("saaaa"), value, valueEquals, "second attr");
    freeVal(value);

    getAttr(r, schema, 2, &value);
    OP_TRUE(stringToValue("i3"), value, valueEquals, "third attr");
    freeVal(value);

    //modify attrs
    setAttr(r, schema, 2, stringToValue("i4"));
    getAttr(r, schema, 2, &value);
    OP_TRUE(stringToValue("i4"), value, valueEquals, "third attr after setting");
    freeVal(value);

    freeRecord(r);
    TEST_DONE();
}

Schema *
testSchema(void) {
    Schema *result;
    char *names[] = {"a", "b", "c"};
    DataType dt[] = {DT_INT, DT_STRING, DT_INT};
    int sizes[] = {0, 4, 0};
    int keys[] = {0};
    int i;
    char **cpNames = (char **) malloc(sizeof(char *) * 3);
    DataType *cpDt = (DataType *) malloc(sizeof(DataType) * 3);
    int *cpSizes = (int *) malloc(sizeof(int) * 3);
    int *cpKeys = (int *) malloc(sizeof(int));

    for (i = 0; i < 3; i++) {
        cpNames[i] = (char *) malloc(2);
        strcpy(cpNames[i], names[i]);
    }
    memcpy(cpDt, dt, sizeof(DataType) * 3);
    memcpy(cpSizes, sizes, sizeof(int) * 3);
    memcpy(cpKeys, keys, sizeof(int));

    result = createSchema(3, cpNames, cpDt, cpSizes, 1, cpKeys);

    return result;
}

Record *
fromTestRecord(Schema *schema, TestRecord in) {
    return testRecord(schema, in.a, in.b, in.c);
}

Record *
testRecord(Schema *schema, int a, char *b, int c) {
    Record *result;
    Value *value;

    TEST_CHECK(createRecord(&result, schema));

    MAKE_VALUE(value, DT_INT, a);
    TEST_CHECK(setAttr(result, schema, 0, value));
    freeVal(value);

    MAKE_STRING_VALUE(value, b);
    TEST_CHECK(setAttr(result, schema, 1, value));
    freeVal(value);

    MAKE_VALUE(value, DT_INT, c);
    TEST_CHECK(setAttr(result, schema, 2, value));
    freeVal(value);

    return result;
}
