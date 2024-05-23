#include "main.h"

// Definiciones locales
void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *, int conexion_memoria);

// Pasa procesos en NEW a READY cuando hay espacio en el semaforo.
/* void planificador_largo_plazo(int *conexion_memoria) */
void *planificador_largo_plazo(void *param)
{
    int conexion_memoria = *((int *)param);

    while (true) {
        // Esperar hasta que haya elementos en NEW, reduciendo el semaforo.
        sem_wait(&sem_elementos_en_new);

        // Una vez que tenemos un elemento en NEW, esperar hasta que haya espacio
        // disponible para agregar un proceso.
        sem_wait(&sem_multiprogramacion);

        // Ver si hay que pausar
        if (planificacion_pausada) {
            log_info(debug_logger, "PLP esperando sem_reanudar_plp");
            sem_wait(&sem_reanudar_plp);
        }

        // Sacar el proceso de NEW y enviarlo a memoria
        t_proceso_nuevo *proceso = squeue_pop(cola_new);

        pthread_mutex_lock(&mutex_conexion_memoria);
        enviar_proceso_nuevo_a_memoria(proceso, conexion_memoria);
        pthread_mutex_unlock(&mutex_conexion_memoria);

        log_info(debug_logger, "Se envió el path %s a memoria para el proceso %d", proceso->path, proceso->pid);

        // Crear el PCB y agregarlo a READY
        // TODO capaz hay que esperar que memoria nos avise que ya cargo el archivo
        t_pcb *pcb = pcb_create(proceso->pid);
        squeue_push(cola_ready, pcb);
        sem_post(&sem_elementos_en_ready);

        // Logs
        log_info(kernel_logger, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid);
        char *lista_pids = obtener_lista_pids(cola_ready);
        log_info(kernel_logger, "Cola Ready cola_ready: [%s]", lista_pids);
        free(lista_pids);

        // Liberar el proceso
        free(proceso->path);
        free(proceso);
    }
}

void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *proceso_nuevo, int conexion_memoria)
{
    enviar_int(MENSAJE_INICIO_PROCESO, conexion_memoria);

    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(proceso_nuevo->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, proceso_nuevo->path, strlen(proceso_nuevo->path) + 1);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}
