#include "State.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FIFO_SC "/tmp/fifo_sc"

#define MAX 5

int main() 
{
    int pc = 0;
    int dispositivo = 0;
    char op = '-';
    State estado = RUNNING;

    signal(SIGINT, SIG_IGN);

    srand(time(NULL) ^ getpid());
    printf("[App %d] iniciado.\n", getpid());

    int fdFifoSc  = open(FIFO_SC, O_WRONLY);

    while (pc < MAX) 
    {
        pc++;
        usleep(500000);  // 0.5 s

        // ~15% de chance de syscall (baixa probabilidade)
        if (rand() % 100 < 15) {
            dispositivo = (rand() % 2) + 1; // 1 ou 2
            op = (rand() % 3 == 0) ? 'R' : (rand() % 3 == 1) ? 'W' : 'X';
        }

        if(pc >= MAX)
            estado = FINISHED;
        else if(dispositivo != 0)
            estado = BLOCKED;

        char buffer[64];
        sprintf(buffer, "%d %d %c %d %s\n", getpid(), dispositivo, op, pc, state2string(estado));
        write(fdFifoSc, buffer, strlen(buffer));

        usleep(500000);  // 0.5 s
    }

    printf("[App %d] terminou. PC final=%d\n",
           getpid(), pc);
    return 0;
}