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
        if (rand() % 100 < 100) 
        {

            OpType tipo = (OpType)(rand() % 5);
            msg->op = tipo;

            int dir = rand() % 3;
            int file = rand() % 5;

            msg->owner = owner;
            msg->pc = pc;
            msg->estado = BLOCKED;
            msg->pid = getpid();
            sprintf(msg->path, "/A%d/dir%d", owner, dir);
            msg->pathLen = strlen(msg->path);
            msg->offset = offsets[rand() % 5];
            
            if (tipo == READ) 
            {
                printf("REQUEST AQUI AIIII %d do APP %d\n", __LINE__, owner);

                char filename[512];   // espaço suficiente
                sprintf(filename, "/file%d", file);
                strcat(msg->path, filename);
                
                printf("[App %d] syscall: read(%s)\n", owner, msg->path);
            }   
            else if (tipo == WRITE) 
            {
                printf("REQUEST AQUI AIIII %d do APP %d\n", __LINE__, owner);

                char filename[512];   // espaço suficiente
                sprintf(filename, "/file%d", file);
                strcat(msg->path, filename);
                
                memset(msg->payload, 'A' + owner , 16);
                msg->payloadLen = 16;
                printf("[App %d] syscall: write(%s)\n", owner, msg->path);
                //TODO payload/offset vazio

            } 
            else if (tipo == ADD_DIR ) 
            {
                printf("REQUEST AQUI AIIII %d do APP %d\n", __LINE__, owner);
                strcpy(msg->dirName, "newDir");
                msg->dirNameLen = strlen(msg->dirName);
                printf("[App %d] syscall: add(%s)\n", owner, msg->path);

            } 
            else if (tipo == REMOVE_DIR) 
            {
                printf("REQUEST AQUI AIIII %d do APP %d\n", __LINE__, owner);

                strcpy(msg->dirName, "toRemoveDir");
                msg->dirNameLen = strlen(msg->dirName);
                printf("[App %d] syscall: rem(%s)\n", owner, msg->path);
            } 
            else if( tipo == LIST_DIR)
            {
                printf("REQUEST AQUI AIIII %d do APP %d\n", __LINE__, owner);

                printf("[App %d] syscall: listdir(%s)\n", owner, msg->path);
            }

            msg->replyReady = 0;
            msg->requestReady = 1;
        }

        if(msg->replyReady)
        {
            //msg->estado = READY;

            if (msg->op == READ) 
            {
                printf("   READ OK: payload recebido = \"");
                for (int i = 0; i < 16; i++)
                    putchar(msg->payload[i]);
                printf("\"\n");
            } 
            else if (msg->op == WRITE) 
            {
                printf("   WRITE OK: 16 bytes foram escritos.\n");
            } 
            else if (msg->op == ADD_DIR) 
            {
                printf("   ADD DIR OK: novo path = %s\n", msg->path);
            } 
            else if (msg->op == REMOVE_DIR) 
            {
                printf("   REMOVE OK: novo path = %s\n", msg->path);
            } 
            else if (msg->op == LIST_DIR) 
            {
                printf("   LISTDIR OK:\n");
                printf("   payload (compactado) = \"");
                for (int i = 0; i < 16; i++)
                    putchar(msg->payload[i]);
                printf("\"\n");
                printf("   strlenPath = %d\n", msg->pathLen);
                printf("   strlenDirName = %d\n", msg->dirNameLen);
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