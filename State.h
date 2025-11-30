#ifndef STATE_H
#define STATE_H

#include <string.h>

typedef enum{
    READY = 0,
    BLOCKED,
    RUNNING,
    FINISHED
} State;

const char* state2string(State s) {
    switch(s) 
    {
        case READY:    return "READY";
        case RUNNING:  return "RUNNING";
        case BLOCKED:  return "BLOCKED";
        case FINISHED: return "FINISHED";
        default:       return "UNKNOWN";
    }
}

State string2state(const char *s) {
    if (!s) return READY;

    if (strcmp(s, "READY") == 0)    return READY;
    if (strcmp(s, "RUNNING") == 0)  return RUNNING;
    if (strcmp(s, "BLOCKED") == 0)  return BLOCKED;
    if (strcmp(s, "FINISHED") == 0) return FINISHED;

    return READY; // fallback simples
}

#endif