#include "ShmMsg.h"
#include "OpType.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
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
    printf("[SFSS] Servidor de gerência de arquivos inicializado\n");

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
        snprintf(ownerDir, sizeof(ownerDir), "%s/A%d", ROOT_DIR, 0);
        mkdir(ownerDir, 0777);
        
        if (req.op == READ)
        {
            printf("[DEBUG][READ] Operação de leitura iniciada\n");

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, req.path);
            printf("[DEBUG][READ] Caminho completo: %s\n", fullpath);

            FILE* f = fopen(fullpath, "rb");
            if(!f)
            {
                printf("[DEBUG][READ] Erro ao abrir arquivo para leitura\n");
                rep.offset = -1;
                rep.payloadLen = 0;
                rep.result_code = -1;
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }
            printf("[DEBUG][READ] Arquivo aberto com sucesso\n");

            fseek(f, 0, SEEK_END);
            long fileSize = ftell(f);
            printf("[DEBUG][READ] Tamanho do arquivo: %ld\n", fileSize);

            if (req.offset >= fileSize)
            {
                printf("[DEBUG][READ] Offset solicitado (%d) >= tamanho do arquivo\n", req.offset);
                rep.offset = -2; 
                rep.payloadLen = 0;
                fclose(f);
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }

            fseek(f, req.offset, SEEK_SET);
            printf("[DEBUG][READ] Lendo 16 bytes a partir do offset %d...\n", req.offset);

            int n = fread(rep.payload, 1, 16, f);
            printf("[DEBUG][READ] Bytes lidos: %d\n", n);

            rep.payloadLen = n;
            rep.result_code = 0;
            rep.offset = n;

            fclose(f);
            printf("[DEBUG][READ] Arquivo fechado\n");
        }
        else if (req.op == WRITE)
        {
            printf("[DEBUG][WRITE] Operação de escrita iniciada\n");

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, req.path);
            printf("[DEBUG][WRITE] Caminho completo: %s\n", fullpath);

            if (req.payloadLen == 0)
            {
                unlink(fullpath);
                rep.result_code = 0;
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }

            if (req.payloadLen == 0 && req.offset == 0)
            {
                printf("[DEBUG][WRITE] Requisição para deletar arquivo\n");

                if (unlink(fullpath) == 0) {
                    printf("[DEBUG][WRITE] Arquivo deletado com sucesso\n");
                    rep.offset = 0;
                } else {
                    printf("[DEBUG][WRITE] Erro ao deletar o arquivo\n");
                    rep.offset = -1;
                }
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }

            printf("[DEBUG][WRITE] Abrindo arquivo para escrita\n");
            FILE* f = fopen(fullpath, "r+b");
            printf("[DEBUG][WRITE] Tentativa r+b: %s\n", f ? "sucesso" : "falha");

            f = fopen(fullpath, "w+b");
            printf("[DEBUG][WRITE] Abrindo com w+b (forçando criação): %s\n", f ? "sucesso" : "falha");

            if (!f) {
                printf("[DEBUG][WRITE] ERRO: fopen retornou NULL\n");
                rep.offset = -1;
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }

            fseek(f, 0, SEEK_END);
            long fileSize = ftell(f);
            printf("[DEBUG][WRITE] Tamanho atual do arquivo: %ld\n", fileSize);

            if(req.offset > fileSize)
            {
                long gap = req.offset - fileSize;
                printf("[DEBUG][WRITE] Offset maior que arquivo. Preenchendo GAP de %ld bytes\n", gap);

                fseek(f, fileSize, SEEK_SET);
                for(long i = 0; i < gap; i++)
                {
                    fputc(0x20, f);
                }
                printf("[DEBUG][WRITE] GAP preenchido\n");
            }

            fseek(f, req.offset, SEEK_SET);
            printf("[DEBUG][WRITE] Escrevendo %d bytes no offset %d\n", req.payloadLen, req.offset);

            int n = fwrite(req.payload, 1, req.payloadLen, f);
            printf("[DEBUG][WRITE] Bytes escritos: %d\n", n);

            if (n != req.payloadLen) {
                printf("[DEBUG][WRITE] ERRO: Nem todos os bytes foram escritos\n");
                rep.offset = -3;
            } else {
                printf("[DEBUG][WRITE] Escrita concluída com sucesso\n");
                rep.offset = req.offset;
            }

            fclose(f);
            printf("[DEBUG][WRITE] Arquivo fechado\n");
        }
        else if (req.op == ADD_DIR)
        {
            printf("[DEBUG][ADD_DIR] Criando diretório\n");

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s%s", ROOT_DIR, req.path, req.dirName);
            printf("[DEBUG][ADD_DIR] Caminho completo: %s\n", fullpath);

            if (mkdir(fullpath, 0777) == 0)
            {
                printf("[DEBUG][ADD_DIR] Diretório criado com sucesso\n");
                rep.result_code = 0;
                snprintf(rep.path, sizeof(rep.path), "%s/%s", req.path, req.dirName);
                rep.pathLen = strlen(rep.path);
            }
            else {
                printf("[DEBUG][ADD_DIR] Erro ao criar diretório\n");
                rep.result_code = -1;
            }  
        }
        else if (req.op == REMOVE_DIR)
        {
            printf("[DEBUG][REMOVE_DIR] Removendo diretório\n");

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s%s", ROOT_DIR, req.path, req.dirName);
            printf("[DEBUG][REMOVE_DIR] Caminho completo: %s\n", fullpath);

            if (rmdir(fullpath) == 0)
            {
                printf("[DEBUG][REMOVE_DIR] Diretório removido com sucesso\n");
                rep.result_code = 0;
                snprintf(rep.path, sizeof(rep.path), "%s", req.path);
                rep.pathLen = strlen(rep.path);
            }
            else {
                printf("[DEBUG][REMOVE_DIR] Erro ao remover diretório\n");
                rep.result_code = -1;
            }
        }
        else if (req.op == LIST_DIR)
        {
            printf("[DEBUG][LIST_DIR] Listando diretório\n");

            char fullpath[128];
            snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, req.dirName);
            printf("[DEBUG][LIST_DIR] Caminho completo: %s\n", fullpath);
            memset(&rep.listDirInfo, 0, sizeof(rep.listDirInfo));

            // Abrindo o diretório corretamente
            DIR* d = opendir(fullpath);  // Usando DIR* ao invés de FILE*
            if (!d)
            {
                rep.result_code = -1;
                sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
                continue;
            }

            rep.listDirInfo.nrnames = 0;
            struct dirent* ent;
            int pos = 0;

            while ((ent = readdir(d)) != NULL)
            {
                // Ignora "." e ".."
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;

                int idx = rep.listDirInfo.nrnames;
                if (idx >= MAX_NAMES) break;

                int nameLen = strlen(ent->d_name);
                if (pos + nameLen >= MAX_ALLDIR) break;

                // Registrar início e fim do nome
                rep.listDirInfo.fstlstpositions[idx][0] = pos;

                memcpy(rep.listDirInfo.allnames + pos, ent->d_name, nameLen);
                pos += nameLen;

                rep.listDirInfo.fstlstpositions[idx][1] = pos;

                // Para descobrir se é diretório, usa stat()
                char tmpPath[512];
                snprintf(tmpPath, sizeof(tmpPath), "%s/%s", fullpath, ent->d_name);
                struct stat st;
                if (stat(tmpPath, &st) == 0 && S_ISDIR(st.st_mode))
                    rep.listDirInfo.isDir[idx] = 1;
                else
                    rep.listDirInfo.isDir[idx] = 0;

                rep.listDirInfo.nrnames++;
            }

            closedir(d);  // Fechar o diretório corretamente

            rep.result_code = 0;
        }
        else
        {
            printf("[DEBUG] Operação desconhecida: %d\n", req.op);
            rep.result_code = -1;
        }

    
        sendto(sock, &rep, sizeof(rep), 0, (struct sockaddr*)&cliAddr, cliLen);
    }
}