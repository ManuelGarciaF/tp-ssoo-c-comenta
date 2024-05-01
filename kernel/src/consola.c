#include "consola.h"

uint32_t ultimo_pid = 0;

void correr_consola(void)
{
    char *input;
    while (true) {
        input = readline("kernel> ");
        if (input) {
            add_history(input);
        }
        if (!input) {
            break;
        }

        // Dividir input en comando y parametro
        char **comando = string_n_split(input, 2, " ");

        if (!strcmp(comando[0], "EJECUTAR_SCRIPT")) {
            assert(0 && "No implementado"); // TODO

        } else if (!strcmp(comando[0], "INICIAR_PROCESO")) {
            if (comando[1] != NULL) {
                char *path_copy = string_duplicate(comando[1]);
                iniciar_proceso(path_copy);
            } else {
                printf("error: El comando necesita un parametro.\n");
            }

        } else if (!strcmp(comando[0], "FINALIZAR_PROCESO")) {
            if (comando[1] != NULL) {
                char *pid_copy = string_duplicate(comando[1]);
                finalizar_proceso(pid_copy);
            } else {
                printf("error: El comando necesita un parametro.\n");
            }
        }
        // TODO

        string_array_destroy(comando);
        free(input);
    }
}

void iniciar_proceso(char *path)
{
    // Guardar tanto el pid como el path, para enviar a memoria
    t_proceso_nuevo *proceso_nuevo = malloc(sizeof(t_proceso_nuevo));
    if (proceso_nuevo == NULL) {
        log_error(debug_logger, "No se pudo alojar espacio para el proceso nuevo");
    }

    proceso_nuevo->pid = ultimo_pid++;
    proceso_nuevo->path = path;

    // Agregarlo a la lista de new
    squeue_push(cola_new, proceso_nuevo);
    sem_post(&sem_elementos_en_new); // Avisamos que hay 1 elemento mas en NEW

    log_info(kernel_logger, "Proceso %d agregado a la cola de new", proceso_nuevo->pid);
    // Ahora depende del planificador a largo plazo.
}

void finalizar_proceso(char *pid)
{
    // TODO
    // free(pid)
}
