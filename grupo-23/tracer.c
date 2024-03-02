#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>

long timestamp_atual(){

    struct timeval ex;

    gettimeofday(&ex, NULL);

    return (ex.tv_sec * 1000 + ex.tv_usec / 1000);
}

int main(int argc, char* argv[]) {

    int pid;

    int fd[2];
    
    if (pipe(fd) == -1) return 1;

    struct timespec time;

    long tempoInicial = 0;

    long tempoFim = 0;

    int clientepServidor = open("clienteServidor", O_WRONLY);
    
    if (clientepServidor == -1) {
        perror("Erro ao abrir o pipe clienteServidor");
        
        exit(1); 
    }

    int servidorpCliente = open("servidorCliente", O_RDONLY);

    if (servidorpCliente == -1) {
        perror("Erro ao abrir o pipe servidorCliente");
        
        exit(1);
    }


    int estado = 1; 

    int mem; 

    if (argc > 3 && strcmp(argv[1], "execute") == 0) {

        if (strcmp(argv[2], "-u") == 0) {

            pid = fork();

            if(pid == 0) {

                close(fd[0]);

                tempoInicial = timestamp_atual();

                write(fd[1], &tempoInicial, sizeof(long));

                close(fd[1]);

                estado = 1;

                write(clientepServidor, &estado, sizeof(int));

                pid = getpid();

                write(clientepServidor, &pid, sizeof(int));

                mem = sizeof(argv[3]) + 1;

                write(clientepServidor, &mem, sizeof(int));

                write(clientepServidor, argv[3], sizeof(char) * mem);

                write(clientepServidor, &tempoInicial, sizeof(long));

                printf("O PID do processo atual é: %d.\n", getpid());

                close(clientepServidor);

                close(servidorpCliente);

                execvp(argv[3], &argv[3]);

                exit(1);        

            } else {

                int status;

                pid = wait(&status);

                if(WIFEXITED(status)) {

                    close(fd[1]);

                    tempoFim = timestamp_atual();

                    read(fd[0], &tempoInicial, sizeof(long));

                    close(fd[0]);

                    printf("O tempo preciso para o programa ser executado foi %ld milissegundos.\n", tempoFim - tempoInicial);

                    estado = 2;

                    write(clientepServidor, &estado, sizeof(int));

                    write(clientepServidor, &pid, sizeof(int));

                    write(clientepServidor, &tempoFim, sizeof(long));
                }

                wait(NULL);
            }
        }
    } else {
        if (argc == 2 && strcmp(argv[1], "status") == 0) {

            estado = 3;

            write(clientepServidor, &estado, sizeof(int));

            while (1) {
                        
                read(servidorpCliente, &mem, sizeof(int));

                if (mem == 0){
                            
                    break;

                }

                char buffer[mem];

                read(servidorpCliente, buffer, mem);

                printf("%s", buffer);
            }
        } else {

            perror("Não foram introduzidos argumentos suficientes ou o comando é inválido.\n");

            _exit(1);
        }
    }
    
    close(clientepServidor);

    close(servidorpCliente);

    return 0;
}