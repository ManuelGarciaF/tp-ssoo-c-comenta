#include "main.h"

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

        // Parsear comando
        char *comando = strtok(input, " ");

        if (!strcmp(comando, "EJECUTAR_SCRIPT")) {
            assert(0 && "No implementado"); // TODO
        } else if (!strcmp(comando, "INICIAR_PROCESO")) {
            char *path_substr = strtok(NULL, " ");

            // Hay que copiar el path, porque strtok devuelve un puntero a la cadena original
            char *path = malloc(strlen(path_substr) + 1);
            strcpy(path, path_substr);

            iniciar_proceso(path);

        } else if (!strcmp(comando, "FINALIZAR_PROCESO")) {
            char *pid_substr = strtok(NULL, " ");

            // Hay que copiar el pid, porque strtok devuelve un puntero a la cadena original
            char *pid = malloc(strlen(pid_substr) + 1);
            strcpy(pid, pid_substr);

            finalizar_proceso(pid);
        }

        // TODO
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

    log_info(kernel_logger, "Proceso %d agregado a la cola de new", proceso_nuevo->pid);
    // Ahora depende del planificador a largo plazo.
}

void finalizar_proceso(char *pid)
{
    // TODO
}
