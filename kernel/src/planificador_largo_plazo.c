#include "planificador_largo_plazo.h"

// Pasa procesos en NEW a READY cuando hay espacio en el semaforo.
void planificador_largo_plazo(int *conexion_memoria)
{
    while (true) {
        // Si no hay elementos en NEW, esperar.
        while (squeue_is_empty(cola_new))
            usleep(50000); // Esperar 50ms idk

        // Esperar que haya espacio disponible para agregar un proceso
        sem_wait(&sem_multiprogramacion);

        t_proceso_nuevo *proceso = squeue_pop(cola_new);
        enviar_proceso_nuevo_a_memoria(proceso, *conexion_memoria);
    }
}

void enviar_proceso_nuevo_a_memoria(t_proceso_nuevo *proceso_nuevo, int conexion_memoria)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(proceso_nuevo->pid), sizeof(uint32_t));
    agregar_a_paquete(paquete, proceso_nuevo->path, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}
