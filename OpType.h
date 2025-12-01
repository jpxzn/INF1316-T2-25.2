#ifndef OPTYPE_H
#define OPTYPE_H

typedef enum{
    READ = 0,
    WRITE,
    ADD_DIR,
    REMOVE_DIR,
    LIST_DIR
} OpType;

#endif