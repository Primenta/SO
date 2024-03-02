#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

long timestamp_atual(){

    struct timeval ex;

    gettimeofday(&ex, NULL);

    return (ex.tv_sec * 1000 + ex.tv_usec / 1000);
}

typedef struct argumentos {
    int pid;
    char* programa;
    long tempoInicial;
    long tempoFinal;
    struct argumentos *next;
} *Argumentos;

Argumentos novoProcesso(int pid) {

    Argumentos new = malloc(sizeof(struct argumentos));

    new->pid = pid;

    new->programa = NULL;

    new->tempoInicial = 0;

    new->tempoFinal = -1;

    new->next = NULL;
    
    return new;
}

Argumentos addProcesso(Argumentos list, Argumentos new) {

    if (list == NULL) 

        return new;

    else {

        Argumentos curr = list;

        while (curr->next != NULL) 

            curr = curr->next;

        curr->next = new;

    }
    return list;
}

Argumentos findProcesso(Argumentos list, int pid) {
    Argumentos curr = list;

    while (curr != NULL) {

        if (curr->pid == pid)

            return curr;

        curr = curr->next;

    }

    return NULL;
}

void ficheiroPID(int pid, const char* nomePrograma, long tempoExecucao) {

    char filename[20];

    sprintf(filename, "%d.txt", pid);

    FILE* file = fopen(filename, "w");

    if (file == NULL) {

        perror("Erro ao criar o arquivo de estado do programa");

        return;
    }

    fprintf(file, "Nome do Programa: %s\n", nomePrograma);

    fprintf(file, "Tempo de execução total: %ld ms\n", tempoExecucao);

    fclose(file);
}

int main(int argc, char* argv[]) {

    char buffer[256];

    int pid;

    mkfifo("clienteServidor", 0777);

    mkfifo("servidorCliente", 0777);

    int clientepServidor = open("clienteServidor", O_RDONLY);

    if (clientepServidor == -1) {

        perror("Erro ao abrir o pipe clienteServidor");

        _exit(1);
    }

    int servidorpCliente = open("servidorCliente", O_WRONLY);

    if (servidorpCliente == -1) {

        perror("Erro ao abrir o pipe servidorCliente");
        
        _exit(1);
    }

    int log = open("log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);

    Argumentos listAux = NULL;

    Argumentos listaF = NULL;

    sprintf(buffer, "PID : Nome do Programa : Tempo de inicialização : Tempo de Finalização\n\n");

    write(log, buffer, strlen(buffer));

    int estado = 0;

    int mem;

    long tempoAux, tempoAtual;

    struct timespec time;

    while (1) {

        read(clientepServidor, &estado, sizeof(int));
        
        if (estado == 1) {

            estado = 0;

            read(clientepServidor, &pid, sizeof(int));

            Argumentos new = novoProcesso(pid);

            read(clientepServidor, &mem, sizeof(int));

            char nomePrograma[mem];

            read(clientepServidor, nomePrograma, sizeof(char) * mem);

            new->programa = strdup(nomePrograma);

            read(clientepServidor, &tempoAux, sizeof(long));

            new->tempoInicial = tempoAux;

            printf("Estatísticas iniciais: %s || %d || %ld\n", nomePrograma, pid, tempoAux);

            listAux = addProcesso(listAux, new);
            
        } else if (estado == 2) {

            estado = 0;

            read(clientepServidor, &pid, sizeof(int));

            read(clientepServidor, &tempoAux, sizeof(long));

            printf("Estatísticas finais: %d || %ld\n", pid, tempoAux);

            listaF = findProcesso(listAux, pid);

            listaF->tempoFinal = tempoAux;

            sprintf(buffer, "%d : %s : %ld : %ld\n", listaF->pid, listaF->programa, listaF->tempoInicial, listaF->tempoFinal);

            write(log, buffer, strlen(buffer));

            ficheiroPID(listaF->pid, listaF->programa, listaF->tempoFinal - listaF->tempoInicial);

        } else if (estado == 3) {

            estado = 0;

            listaF = listAux;

            while (listaF != NULL) {

                if (listaF->tempoFinal == -1) {

                    tempoAtual = timestamp_atual();

                    sprintf(buffer, "%s || %d || %ld ms (Loading)\n", listaF->programa, listaF->pid, tempoAtual - listaF->tempoInicial);

                } else {

                    sprintf(buffer, "%s || %d || %ld ms (Finished)\n", listaF->programa, listaF->pid, listaF->tempoFinal - listaF->tempoInicial);

                }
                mem = strlen(buffer) + 1;

                write(servidorpCliente, &mem, sizeof(int));

                write(servidorpCliente, buffer, mem);

                listaF = listaF->next;
            }
            mem = 0;

            write(servidorpCliente, &mem, sizeof(int));
        }
    }

    close(clientepServidor);

    close(servidorpCliente);

    close(log);

    return 0;
}