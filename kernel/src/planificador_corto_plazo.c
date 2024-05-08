#include "planificador_corto_plazo.h"

// Pasa procesos de READY a EXEC
void planificador_corto_plazo(t_parametros_pcp *params)
{
    // Empieza en 0 para bloquear hasta que le digamos.
    sem_init(&sem_comenzar_reloj, 0, 0);

    pthread_t hilo_reloj_rr;
    // Crear hilo de reloj para desalojar procesos en RR.
    if (algoritmo_planificacion == RR) {
        int iret = pthread_create(&hilo_reloj_rr, NULL, (void *)reloj_rr, NULL);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
        }
    }

    while (true) {
        // Esperar que haya elementos en ready.
        sem_wait(&sem_elementos_en_ready);
        // Esperar que no haya procesos ejecutando.
        sem_wait(&sem_proceso_en_ejecucion);

        // Por FIFO y RR siempre tomamos el primero
        t_pcb *pcb_a_ejecutar = squeue_pop(cola_ready);
        // Enviamos el pcb a CPU.
        pcb_send(pcb_a_ejecutar, params->conexion_cpu_dispatch);

        log_info(kernel_logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb_a_ejecutar->pid);

        pcb_destroy(pcb_a_ejecutar);

        // Si estamos en RR o VRR, iniciar el reloj.
        if (algoritmo_planificacion == RR || algoritmo_planificacion == VRR) {
            sem_post(&sem_comenzar_reloj);
        }

        // En este punto, se esta ejecutando el proceso, esperamos que interrumpa
        // y nos devuelvan el pcb actualizado y el motivo.
        t_pcb *pcb_actualizado = NULL;
        char *nombre_recurso_interfaz = NULL;
        t_motivo_desalojo motivo = recibir_pcb(params->conexion_cpu_dispatch, pcb_actualizado, nombre_recurso_interfaz);

        switch (motivo) {
        case FIN_QUANTUM:
            // Lo volvemos a agregar a la cola de READY
            // TODO ver quantum para VRR
            squeue_push(cola_ready, pcb_actualizado);

            // Logs
            log_info(kernel_logger, "PID: %d - Desalojado por fin de Quantum", pcb_actualizado->pid);
            log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", pcb_actualizado->pid);
            char *lista_pids = obtener_lista_pids(cola_ready);
            log_info(kernel_logger, "Cola Ready cola_ready: [%s]", lista_pids);
            free(lista_pids);
            break;

        case FIN_PROCESO:
            // Habilitar 1 espacio en el grado de multiprogramacion
            sem_post(&sem_multiprogramacion);

            log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_actualizado->pid);
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: SUCCESS", pcb_actualizado->pid);
            break;

        // TODO
        case WAIT_RECURSO:
        case SIGNAL_RECURSO:
        case IO:
            break;
        }
    }

    sem_destroy(&sem_comenzar_reloj);
}

void reloj_rr(int conexion_cpu_interrupt)
{
    while (true) {
        sem_wait(&sem_comenzar_reloj);
        // Multiplicamos por 1000 porque toma microsegundos, quantum esta en milisegundos.
        usleep(quantum * 1000);
        if (!proceso_desalojo_previamente) {
            // Desalojar por fin de Quantum
            desalojar_proceso(conexion_cpu_interrupt);
        }
    }
}

void desalojar_proceso(int conexion_interrupt)
{
    // TODO enviar un mensaje al CPU para desalojar
}

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb *pcb_actualizado, char *nombre_recurso_interfaz)
{
    t_list *elementos = recibir_paquete(conexion_cpu_dispatch);
    pcb_actualizado = list_get(elementos, 0);
    t_motivo_desalojo *motivo = list_get(elementos, 1);

    nombre_recurso_interfaz =
        (*motivo == WAIT_RECURSO || *motivo == SIGNAL_RECURSO || *motivo == IO) ? list_get(elementos, 2) : NULL;

    list_destroy(elementos);

    return *motivo;
}
