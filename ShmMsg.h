#ifndef SHMMSG_H
#define SHMMSG_H

#include <unistd.h>
#include "OpType.h"
#include "State.h"

#define MAX_NAMES 40
#define MAX_ALLDIR 1024

typedef struct {
    int requestReady;    // Ax -> Kernel
    int replyReady;      // Kernel -> Ax

    int pc;
    pid_t pid;
    State estado;

    int owner;           // qual Ax
    OpType op;         // READ/WRITE/ADD/REM/LISTDIR

    char path[32];

    int  pathLen;
    int  offset;         // para read/write

    char payload[16];       // write: dados; read: resposta
    int payloadLen;

    char dirName[32];   // DirName / Nome (add/rem)
    int  dirNameLen;

    struct {
        int nrnames;       // número de nomes
        char allnames[MAX_ALLDIR]; // lista de nomes concatenados
        int fstlstpositions[MAX_NAMES][2]; // posições iniciais/finais de cada nome
        char isDir[MAX_NAMES]; // indica se é diretório (1) ou arquivo (0)
    } listDirInfo;

    int  result_code;    // offset/strlen/nrnames ou < 0 erro
} ShmMsg;

#endif