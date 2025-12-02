#include "ShmMsg.h"
#include "OpType.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ROOT_DIR "root_dir"

int main(void) {
    system("rm -rf root_dir");
    mkdir(ROOT_DIR, 0777);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(6000);

    bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr));
    printf("[SFS] Servidor de gerência de arquivos inicializado\n");

    while(1)
    {
        ShmMsg req;
        struct sockaddr_in cliAddr;
        socklen_t cliLen = sizeof(cliAddr);

        int n = recvfrom(sock, &req, sizeof(req), 0, (struct sockaddr*)&cliAddr, &cliLen);

        if(n <= 0) continue;

        printf("[SFS] Recebido request op=%d owner=%d\n", req.op, req.owner);

        ShmMsg rep;
        memset(&rep, 0, sizeof(rep));

        rep.owner = req.owner;
        rep.op = req.op;

        memset(rep.payload, 0, 16);
        rep.payloadLen = 0;
    
        char ownerDir[256];
        snprintf(ownerDir, sizeof(ownerDir), "%s/A%d", ROOT_DIR, req.owner);
        mkdir(ownerDir, 0777);
        
        if (req.op == READ)
        {
            char fullpath[256];
            sprintf(fullpath, "%s%s", ROOT_DIR, req.path);

            FILE* f = fopen(fullpath, "rb");
            if(!f)
            {
                rep.offset = -1;
                rep.payloadLen = 0;
                continue;
            }
            
            fseek(f, 0, SEEK_END);
            long fileSize = ftell(f);
            
            if (req.offset >= fileSize)
            {
                rep.offset = -2; 
                rep.payloadLen = 0;
                fclose(f);
                continue;
            }

            fseek(f, req.offset, SEEK_SET);

            int n = fread(rep.payload, 1, 16, f);
            rep.payloadLen = n;
            rep.result_code = 0;
            rep.offset = n;

            fclose(f);
        }
        else if (req.op == WRITE)
        {
            char fullpath[256];
            sprintf(fullpath, "%s%s", ROOT_DIR, req.path);

            if (req.payloadLen == 0 && req.offset == 0)
            {
                if (unlink(fullpath) == 0)
                    rep.offset = 0;
                else
                    rep.offset = -1; 
                continue;
            }

            FILE* f = fopen(fullpath, "r+b");
            f = fopen(fullpath, "w+b");

            fseek(f, 0, SEEK_END);
            long fileSize = ftell(f);

            if(req.offset > fileSize)
            {
                long gap = req.offset - fileSize;
                fseek(f, fileSize, SEEK_SET);

                for(long i = 0; i < gap; i++)
                {
                    fputc(0x20, f);
                }
            }
            
            fseek(f, req.offset, SEEK_SET);
            int n = fwrite(req.payload, 1, req.payloadLen, f);

            if (n != req.payloadLen)
                rep.offset = -3;  
            else
                rep.offset = req.offset;

            fclose(f);
        }
        else if (req.op == ADD_DIR)
        {
            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s%s/%s", ROOT_DIR, req.path, req.dirName);

            if (mkdir(fullpath, 0777) == 0)
            {
                rep.result_code = 0;
                snprintf(rep.path, sizeof(rep.path), "%s/%s", req.path, req.dirName);
                rep.pathLen = strlen(rep.path);
            }
            else {
                rep.result_code = -1;  
            }  
        }    
        else if (req.op == REMOVE_DIR)
        {
            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ROOT_DIR, req.path);
        
            if (rmdir(fullpath) == 0)
            {
                rep.result_code = 0;
                snprintf(rep.path, sizeof(rep.path), "%s", req.path);
                rep.pathLen = strlen(rep.path);
            }
            else rep.result_code = -1;
        }
        else if (req.op == LIST_DIR)
        {
            //TODO
            continue;
        }
        else
        {
            rep.result_code = -1;
        }
    
        sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
    }
}