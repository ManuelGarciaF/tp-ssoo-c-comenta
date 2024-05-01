#include "planificador_largo_plazo.h"

// Pasa procesos en NEW a READY cuando hay espacio en el semaforo.
void planificador_largo_plazo(int *conexion_memoria)
{
    while (true) {
        // Esperar hasta que haya elementos en NEW, reduciendo el semaforo.
        sem_wait(&sem_elementos_en_new);

        // Una vez que tenemos un elemento en NEW, esperar hasta que haya espacio
        // disponible para agregar un proceso.
        sem_wait(&sem_multiprogramacion);

        // Sacar el proceso de NEW y enviarlo a memoria
        t_proceso_nuevo *proceso = squeue_pop(cola_new);
        enviar_proceso_nuevo_a_memoria(proceso, *conexion_memoria);

        log_info(debug_logger, "Se envió el proceso %d con path: %s a memoria", proceso->pid, proceso->path);

        // Crear el PCB y agregarlo a READY
        // TODO capaz hay que esperar que memoria nos avise que ya cargo el archivo
        t_pcb *pcb = pcb_create(proceso->pid);
        squeue_push(cola_ready, pcb);

        // Liberar el proceso
        free(proceso->path);
        free(proceso);
    }
}

void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *proceso_nuevo, int conexion_memoria)
{
    enviar_mensaje(MENSAJE_INICIO_PROCESO, conexion_memoria);
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(proceso_nuevo->pid), sizeof(uint32_t));

    printf("%s\n", proceso_nuevo->path);
    agregar_a_paquete(paquete, proceso_nuevo->path, strlen(proceso_nuevo->path) + 1);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}
