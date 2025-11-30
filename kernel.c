#include "Procinfo.h"
#include "State.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define FIFO_IRQ "/tmp/fifo_irq"
#define FIFO_SC "/tmp/fifo_sc"
#define NPROC 5

ProcInfo procs[NPROC];
pid_t FilaD1[NPROC], FilaD2[NPROC];
int tamFilaD1 = 0;
int tamFilaD2 = 0;
int currentProc = 0;
pid_t interController;

sig_atomic_t paused = 0;
void pauseSignal(int sig) 
{ paused = !paused; }

int proximo_ready(int current) {
    for (int k = 1; k <= NPROC; k++) {
        int j = (current + k) % NPROC;
        if (procs[j].estado == READY) return j;
    }
    return -1;
}

void desbloqueia_processo(int dispositivo) 
{
    if(dispositivo == 1)
    {
        if(tamFilaD1 == 0) return;
        else
        {
            for(int i = 0; i < NPROC; i++)
            {
                if(procs[i].pid == FilaD1[0])
                {
                    procs[i].estado = READY;
                    procs[i].dispositivo = 0;
                    procs[i].operacao = '-';
                    break;
                }
            }

            for(int i = 1; i < tamFilaD1; i++)
                FilaD1[i - 1] = FilaD1[i];
            tamFilaD1--;
        }
    }
    else
    {
        if(tamFilaD2 == 0) return;
        else
        {
            for(int i = 0; i < NPROC; i++)
            {
                if(procs[i].pid == FilaD2[0])
                {
                    procs[i].estado = READY;
                    procs[i].dispositivo = 0;
                    procs[i].operacao = '-';
                    break;
                }
            }
            
            for(int i = 1; i < tamFilaD2; i++)
                FilaD2[i - 1] = FilaD2[i];
            tamFilaD2--;
        }
    }
}

void escalona_proximo()
{
    if(procs[currentProc].estado == RUNNING && procs[currentProc].pid > 0) 
    {
        kill(procs[currentProc].pid, SIGSTOP);
        procs[currentProc].estado = READY;
    }
    
    int nextReady = proximo_ready(currentProc);
    if(nextReady < 0) return;

    currentProc = nextReady;
    procs[currentProc].estado = RUNNING;
    kill(procs[currentProc].pid, SIGCONT);
}


int main()
{

    signal(SIGINT, pauseSignal);
    unlink(FIFO_SC);
    unlink(FIFO_IRQ);
    mkfifo(FIFO_SC, S_IRUSR | S_IWUSR);
    mkfifo(FIFO_IRQ, S_IRUSR | S_IWUSR);
    
    int fdFifoIrq = open(FIFO_IRQ, O_RDONLY | O_NONBLOCK);
    int fdFifoSc  = open(FIFO_SC,  O_RDONLY | O_NONBLOCK);

    interController = fork();
    if(interController == 0)
    { 
        execl("./interController", "./interController", NULL);
        exit(1);
    }

    kill(interController, SIGSTOP);

    for (int i = 0; i < NPROC; i++)
    {
        procs[i].pid =0;
        procs[i].estado = READY;
        procs[i].dispositivo = 0;
        procs[i].operacao = '-';
        procs[i].acessos_D1 = 0;
        procs[i].acessos_D2 = 0;
        procs[i].pc = 0;
    }

    for(int i = 0; i < NPROC; i++)
    {
        pid_t pid = fork();
        if(pid == 0) 
        { 
            execl("./app", "./app", NULL);
            exit(1);
        } 
        else 
        {
            procs[i].pid = pid;
            kill(procs[i].pid, SIGSTOP);
        }
    }

    int irqBuffer;
    char scBuffer[64];

    kill(interController, SIGCONT);

    int hasProcessesRunning = 1;
    while(hasProcessesRunning)
    {
        if(read(fdFifoIrq, &irqBuffer, sizeof(int)) > 0)
        {
            switch (irqBuffer) {
                    case 0:
                        printf("[KernelSim] IRQ0 recebido: Troca de processo.\n");
                        escalona_proximo();
                        break;
                    case 1:
                        printf("[KernelSim] IRQ1 recebido: operação em D1 terminou.\n");
                        desbloqueia_processo(1);
                        break;
                    case 2:
                        printf("[KernelSim] IRQ2 recebido: operação em D2 terminou.\n");
                        desbloqueia_processo(2);
                        break;
            }
        }
        if(read(fdFifoSc, scBuffer, sizeof(scBuffer)) > 0)
        {
            char estado[64];
            int pid, dispositivo, pc;
            char operacao;
            sscanf(scBuffer, "%d %d %c %d %s", &pid, &dispositivo, &operacao, &pc, estado);

            for(int i = 0; i < NPROC; i++)
            {
                if(procs[i].pid == pid)
                {
                    procs[i].pc = pc;
                    procs[i].estado = string2state(estado); 
                    procs[i].operacao = operacao;
                    procs[i].dispositivo = dispositivo;

                    if(pc >= 5)
                        procs[i].estado = FINISHED;

                    if(procs[i].estado == FINISHED)
                    {
                        procs[i].operacao = '-';
                        procs[i].dispositivo = 0;
                        procs[i].acessos_D1 = 0;
                        procs[i].acessos_D2 = 0;
                    }

                    if(dispositivo == 1)
                    {
                        FilaD1[tamFilaD1++] = pid;
                        procs[i].acessos_D1++;
                    }
                    else if(dispositivo == 2)
                    {
                        FilaD2[tamFilaD2++] = pid;
                        procs[i].acessos_D2++;
                    }

                    if(procs[i].estado == BLOCKED)
                        kill(procs[i].pid, SIGSTOP);

                    break;
                }
            }

            printf("[KernelSim] System Call recebida: Processo %d requisitou operação %c em D%d.\n", pid, operacao, dispositivo);
        }
        if(paused){
            kill(interController, SIGSTOP);
            for(int i = 0; i < NPROC; i++)
                kill(procs[i].pid, SIGSTOP);

            // Status
            printf("%-8s %-10s %-6s %-6s %-6s %-10s %-10s\n",
            "PID", "ESTADO", "PC", "D1", "D2", "DISP", "OPERACAO");
            printf("----------------------------------------------------------\n");

            for (int i = 0; i < NPROC; i++) 
            {
                printf("%-8d    %-10s %-6d %-6d %-6d %-10d %-10c\n",
                    procs[i].pid, state2string(procs[i].estado), procs[i].pc,
                    procs[i].acessos_D1, procs[i].acessos_D2,
                    procs[i].dispositivo, procs[i].operacao);
            }
            
            printf("[KernelSim] Kernel pausado. Pressione Ctrl+C para continuar...\n");
            pause();
        }
        else{
            kill(interController, SIGCONT);
            for(int i = 0; i < NPROC; i++)
            {
                if(procs[i].estado == RUNNING)
                    kill(procs[i].pid, SIGCONT);
            }
        }

        for(int i = 0; i < NPROC; i++)
        {
            if(procs[i].estado != FINISHED)
            {
                hasProcessesRunning = 1;
                break;
            }
            else
                hasProcessesRunning = 0;
        }

    }
   
    kill(interController, SIGUSR1);

    for(int i = 0; i < NPROC; i++)
        waitpid(0, NULL, 0);
    close(fdFifoIrq);
    close(fdFifoSc);
    
    unlink(FIFO_IRQ);
    unlink(FIFO_SC);
    return 0;
}