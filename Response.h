#ifndef RESPONSE_H
#define RESPONSE_H

#include "OpType.h"
#include "ShmMsg.h"

typedef struct {
    ShmMsg rep;   // TODO - criar uma estrutura UdpResponse
    OpType op;
    int valid; 
} Response;

#endif