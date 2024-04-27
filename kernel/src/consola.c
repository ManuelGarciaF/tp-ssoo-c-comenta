#include "main.h"

int ultimo_pid = 0;

void correr_consola(void) {
    char *input;
    while (true) {
        input = readline("kernel> ");
        if (input) {
            add_history(input);
        }
        if (!input) {
            break;
        }

        // Parsear comando
        char *comando = strtok(input, " ");

        if (!strcmp(comando, "EJECUTAR_SCRIPT")) {
            assert(0 && "No implementado"); // TODO
        } else if (!strcmp(comando, "INICIAR_PROCESO")) {
            char *path = strtok(NULL, " ");
            iniciar_proceso(path);
        } else if (!strcmp(comando, "FINALIZAR_PROCESO")) {
            char *pid = strtok(NULL, " ");
            finalizar_proceso(pid);
        }

        // TODO
        free(input);
    }
}

void iniciar_proceso(char *path) {
    // Crear un nuevo pcb
    t_pcb *pcb = pcb_create(ultimo_pid);
    log_info(kernel_logger, "Se crea el proceso %d en NEW", ultimo_pid);
    ultimo_pid++;

    // Agregarlo a la lista de new
    squeue_push(cola_new, pcb);
    // Ahora depende del planificador a largo plazo.
}

void finalizar_proceso(char *pid) {
    // TODO
}
