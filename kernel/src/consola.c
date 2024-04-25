#include "main.h"

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
    // TODO
}

void finalizar_proceso(char *pid) {
    // TODO
}
