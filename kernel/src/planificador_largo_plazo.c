#include "main.h"

// Definiciones locales
void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *);

// Pasa procesos en NEW a READY cuando hay espacio.
void *planificador_largo_plazo(void *param)
{
    while (true) {
        // Esperar hasta que haya elementos en NEW.
        sem_wait(&sem_elementos_en_new);
        t_proceso_nuevo *proceso_inicial = (squeue_is_empty(cola_new)) ? NULL : squeue_peek(cola_new);

        // Una vez que tenemos un elemento en NEW, esperar hasta que haya espacio
        // disponible para agregar un proceso.
        sem_wait(&sem_multiprogramacion);

        // Tomar el permiso para agregar procesos a ready
        sem_wait(&sem_entrada_a_ready);

        // Ver que mientras esperabamos no nos hayan sacado el proceso en new (eliminado por consola)
        // Obtener el primer proceso en NEW
        t_proceso_nuevo *proceso = (squeue_is_empty(cola_new)) ? NULL : squeue_peek(cola_new);
        // Si cambio o no tenemos ningun proceso, no hacer nada
        if (proceso_inicial != proceso || proceso_inicial == NULL || proceso == NULL) {
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

        // Esperar que memoria nos avise que cargo el proceso.
        pthread_mutex_lock(&mutex_conexion_memoria);
        bool err = false;
        uint32_t respuesta_memoria = recibir_int(conexion_memoria, &err);
        pthread_mutex_unlock(&mutex_conexion_memoria);
        if (err || respuesta_memoria != MENSAJE_OP_TERMINADA) {
            log_error(debug_logger, "Memoria no pudo cargar el proceso");
            abort();
        }

        // Crear el PCB y agregarlo a READY
        t_pcb *pcb = pcb_create(proceso->pid);
        // Poner el quantum inicial
        pcb->quantum = QUANTUM;
        squeue_push(cola_ready, pcb);

        // Liberar el permiso para agregar procesos a ready
        sem_post(&sem_entrada_a_ready);

        // Logs
        log_info(kernel_logger, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid);
        log_cola_ready();

        sem_post(&sem_elementos_en_ready);

        // Liberar el proceso
        free(proceso->path);
        free(proceso);
    }
}

void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *proceso_nuevo)
{
    pthread_mutex_lock(&mutex_conexion_memoria);

    enviar_int(OPCODE_INICIO_PROCESO, conexion_memoria);

    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(proceso_nuevo->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, proceso_nuevo->path, strlen(proceso_nuevo->path) + 1);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    pthread_mutex_unlock(&mutex_conexion_memoria);
}
