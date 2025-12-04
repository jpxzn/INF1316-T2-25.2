#include "State.h"
#include "ShmMsg.h"
#include "OpType.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h> 

#define MAX 5

int main(int argc, char *argv[]) 
{
    int pc = 0;
    int owner = atoi(argv[1]);
    State estado = RUNNING;
    int offsets[] = {0,16,32,48,64};

    signal(SIGINT, SIG_IGN);

    srand(time(NULL) ^ getpid());
    printf("[App %d] iniciado.\n", getpid());

    char name[32];
    sprintf(name, "/A%d", owner);
    int fd = shm_open(name, O_RDWR, 0666);
    ShmMsg* msg = mmap(NULL, sizeof(ShmMsg),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);

    while (pc < MAX) 
    {
        pc++;
        msg->pc = pc;
        usleep(500000);  // 0.5 s

        // ~10% de chance de syscall
        if (rand() % 100 < 10) 
        {

            OpType tipo = (OpType)(rand() % 5);
            msg->op = tipo;
            msg->owner = owner;
            msg->pc = pc;
            msg->estado = BLOCKED;
            msg->pid = getpid();
            int ownDir = rand() % 5;
            int procdir = ownDir == 0? owner : 0;
            sprintf(msg->path, "/A%d", procdir);
            msg->pathLen = strlen(msg->path);
            msg->offset = offsets[rand() % 5];
            
            int file = rand() % 5;
            char filename[32];   // espaço suficiente
            sprintf(filename, "/file%d", file);
            
            int dir = rand() % 3;
            char nameDir[32];   // espaço suficiente
            sprintf(nameDir, "/dir%d", dir + 1);
            
            
            if (tipo == READ) 
            {
                strcat(msg->path, filename);
                printf("[App %d] syscall: read(%s)\n", owner, msg->path);
            }   
            else if (tipo == WRITE) 
            {   
                int empty_write = rand() % 5 == 0;
                if (empty_write)
                    msg->payloadLen = 0;
                else{
                    strcat(msg->path, filename);
                    memset(msg->payload, 'A' + (rand() % 5) , 16);
                    msg->payloadLen = 16;
                }
                printf("[App %d] syscall: write(%s)\n", owner, msg->path);
            } 
            else if (tipo == ADD_DIR ) 
            {
                strcpy(msg->dirName, nameDir);
                msg->dirNameLen = strlen(msg->dirName);
                printf("[App %d] syscall: add(%s)\n", owner, msg->path);

            } 
            else if (tipo == REMOVE_DIR) 
            {
                strcpy(msg->dirName, nameDir);
                msg->dirNameLen = strlen(msg->dirName);
                printf("[App %d] syscall: rem(%s)\n", owner, msg->path);
            } 
            else if( tipo == LIST_DIR)
            {
                sprintf(msg->dirName, "/A%d", procdir);
                printf("[App %d] syscall: listdir(%s)\n", owner, msg->path);
            }

            msg->replyReady = 0;
            msg->requestReady = 1;
        }

        if(msg->replyReady)
        {
            if(msg->result_code < 0)
            {
                printf("   ERRO na operação %d\n", msg->op);
            } 
            else if (msg->op == READ) 
            {
                printf("   READ OK: payload recebido = \"");
                for (int i = 0; i < 16; i++)
                    putchar(msg->payload[i]);
                printf("\"\n");
            } 
            else if (msg->op == WRITE) 
            {
                printf("   WRITE OK: Mensagem escrita.\n");
            } 
            else if (msg->op == ADD_DIR) 
            {
                printf("   ADD DIR OK: novo path = %s\n", msg->path);
            } 
            else if (msg->op == REMOVE_DIR) 
            {
                printf("   REMOVE OK: path removido = %s\n", msg->path);
            } 
            else if (msg->op == LIST_DIR) 
            {
                printf("   LISTDIR OK:\n");
                for (int i = 0; i < msg->listDirInfo.nrnames; i++) {
                    int start = msg->listDirInfo.fstlstpositions[i][0];
                    int end = msg->listDirInfo.fstlstpositions[i][1];
                    char name[256];
                    memcpy(name, &msg->listDirInfo.allnames[start], end - start);
                    name[end - start] = '\0';
                    printf("      %s %s\n", name, msg->listDirInfo.isDir[i] ? "(DIR)" : "(FILE)");
                }
            }

            msg->replyReady = 0;
            msg->requestReady = 0;
            msg->result_code = 0;

            memset(msg->path, 0, sizeof(msg->path));
            memset(msg->dirName, 0, sizeof(msg->dirName));
            memset(msg->payload, 0, sizeof(msg->payload));

            msg->payloadLen = 0;

            msg->offset = 0;
            msg->pathLen = 0;
            msg->dirNameLen = 0;
        }

        usleep(500000);  // 0.5 s
    }

    printf("[App %d] terminou. PC final=%d\n",
           getpid(), pc);
    return 0;
}