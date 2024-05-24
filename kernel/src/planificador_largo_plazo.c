#include "main.h"

// Definiciones locales
void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *);

// Pasa procesos en NEW a READY cuando hay espacio.
void *planificador_largo_plazo(void *param)
{
    while (true) {
        // Esperar hasta que haya elementos en NEW.
        sem_wait(&sem_elementos_en_new);
        t_proceso_nuevo *proceso_inicial = squeue_peek(cola_new);

        // Una vez que tenemos un elemento en NEW, esperar hasta que haya espacio
        // disponible para agregar un proceso.
        sem_wait(&sem_multiprogramacion);

        // Tomar el permiso para agregar procesos a ready
        sem_wait(&sem_entrada_a_ready);

        // Ver que mientras esperabamos no nos hayan sacado el proceso en new (eliminado por consola)
        // Obtener el primer proceso en NEW
        t_proceso_nuevo *proceso = squeue_peek(cola_new);
        if (proceso_inicial != proceso) { // Si cambio, no hacer nada
            // Devolvemos el espacio que no utilizamos
            sem_post(&sem_multiprogramacion);
            // Liberar el permiso para agregar procesos a ready
            sem_post(&sem_entrada_a_ready);
            continue;
        }

        // Sacar el proceso que vimos antes de la cola
        squeue_pop(cola_new);

        enviar_proceso_nuevo_a_memoria(proceso);

        log_info(debug_logger, "Se envió el path %s a memoria para el proceso %d", proceso->path, proceso->pid);

        // Crear el PCB y agregarlo a READY
        // TODO capaz hay que esperar que memoria nos avise que ya cargo el archivo
        t_pcb *pcb = pcb_create(proceso->pid);
        squeue_push(cola_ready, pcb);

        // Liberar el permiso para agregar procesos a ready
        sem_post(&sem_entrada_a_ready);

        sem_post(&sem_elementos_en_ready);

        // Logs
        log_info(kernel_logger, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid);
        char *lista_pids = obtener_lista_pids_pcb(cola_ready);
        log_info(kernel_logger, "Cola Ready cola_ready: [%s]", lista_pids);
        free(lista_pids);


        // Liberar el proceso
        free(proceso->path);
        free(proceso);
    }
}

void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *proceso_nuevo)
{
    pthread_mutex_lock(&mutex_conexion_memoria);

    enviar_int(MENSAJE_INICIO_PROCESO, conexion_memoria);

    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(proceso_nuevo->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, proceso_nuevo->path, strlen(proceso_nuevo->path) + 1);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    pthread_mutex_unlock(&mutex_conexion_memoria);
}
