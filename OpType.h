#ifndef OPTYPE_H
#define OPTYPE_H

typedef enum{
    NONE = 0,
    READ,
    WRITE,
    ADD_DIR,
    REMOVE_DIR,
    LIST_DIR
} OpType;

#endif