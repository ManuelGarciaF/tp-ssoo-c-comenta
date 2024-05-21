#include "consola.h"

uint32_t ultimo_pid = 0;

void correr_consola(void)
{
    char *input;
    while (true) {
        // Esperar unos milisegundos para que no se desordene por los logs
        // TODO ver si esto es ilegal
        usleep(50 * 1000);
        input = readline("kernel> ");
        if (input) {
            add_history(input);
        }
        if (!input) {
            break;
        }

        ejecutar_comando(input);

        free(input);
    }
}

void ejecutar_comando(char const *linea)
{
    // Dividir input en comando y parametro
    char **comando = string_n_split((char *)linea, 2, " ");

    if (!strcmp(comando[0], "EJECUTAR_SCRIPT")) {
        assert(0 && "No implementado"); // TODO

    } else if (!strcmp(comando[0], "INICIAR_PROCESO")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
        }
        iniciar_proceso(comando[1]);

    } else if (!strcmp(comando[0], "FINALIZAR_PROCESO")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        finalizar_proceso(comando[1]);

    } else if (!strcmp(comando[0], "DETENER_PLANIFICACION")) {
        pausar_planificacion();

    } else if (!strcmp(comando[0], "INICIAR_PLANIFICACION")) {
        if (planificacion_pausada) {
            reanudar_planificacion();
        }

    } else if (!strcmp(comando[0], "MULTIPROGRAMACION")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        actualizar_grado_multiprogramacion(atoi(comando[1]));

    } else if (!strcmp(comando[0], "PROCESO_ESTADO")) {
        assert(false && "No Implementado");
    } else {
        printf("Comando no reconocido\n");
    }


    string_array_destroy(comando);
}

void iniciar_proceso(char const *path)
{
    // Guardar tanto el pid como el path, para enviar a memoria
    t_proceso_nuevo *proceso_nuevo = malloc(sizeof(t_proceso_nuevo));
    assert(proceso_nuevo != NULL);

    proceso_nuevo->pid = ultimo_pid++;
    // Hay que duplicarlo para poder liberar el comando despues
    proceso_nuevo->path = string_duplicate((char *)path);

    // Agregarlo a la lista de new
    squeue_push(cola_new, proceso_nuevo);
    sem_post(&sem_elementos_en_new); // Avisamos que hay 1 elemento mas en NEW

    log_info(kernel_logger, "Proceso %d agregado a la cola de new", proceso_nuevo->pid);
    // Ahora depende del planificador a largo plazo.
}

void finalizar_proceso(char const *pid)
{
    // TODO
    assert(false && "No implementado");
}

void actualizar_grado_multiprogramacion(int nuevo)
{
    int diferencia = nuevo - grado_multiprogramacion;

    grado_multiprogramacion = nuevo;

    // Hay que agregar espacio
    if (diferencia > 0) {
        for (int i = 0; i < diferencia; i++) {
            sem_post(&sem_multiprogramacion);
        }
    } else if (diferencia < 0) { // Hay que quitar espacio (diferencia es negativo)
        int procesos_extra = -diferencia;
        procesos_extra_multiprogramacion += procesos_extra;
    }
}
