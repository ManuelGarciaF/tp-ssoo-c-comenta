#include "planificador_corto_plazo.h"

bool planificar_nuevo_proceso;

// Pasa procesos de READY a EXEC
void planificador_corto_plazo(t_parametros_pcp *params)
{
    // Empieza en 0 para bloquear hasta que le digamos.
    sem_init(&sem_comenzar_reloj, 0, 0);

    // TODO crear el hilo cada vez luego de enviar el pcb, para poder matarlo si es necesario.
    pthread_t hilo_reloj_rr;
    // Crear hilo de reloj para desalojar procesos en RR.
    if (algoritmo_planificacion == RR) {
        int iret = pthread_create(&hilo_reloj_rr, NULL, (void *)reloj_rr, NULL);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
        }
    }

    planificar_nuevo_proceso = true;
    while (true) {
        // No siempre hay que enviar un proceso nuevo segun el algoritmo de planificacion,
        // a veces devolvemos la ejecucion al proceso devuelto.
        if (planificar_nuevo_proceso) {
            // Esperar que haya elementos en ready.
            sem_wait(&sem_elementos_en_ready);

            // Ver si hay que pausar (siempre despues de bloqueo)
            if (planificacion_pausada) {
                log_info(debug_logger, "PCP esperando sem_reanudar_pcp");
                sem_wait(&sem_reanudar_pcp);
            }

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
        }

        // En este punto, se esta ejecutando el proceso, esperamos que interrumpa y
        // nos devuelvan el pcb actualizado y el motivo.

        t_pcb *pcb_recibido = NULL;
        t_motivo_desalojo motivo = recibir_pcb(params->conexion_cpu_dispatch, &pcb_recibido); // Bloqueante

        // Ver si hay que pausar (siempre despues de bloqueo)
        if (planificacion_pausada) {
            log_info(debug_logger, "PCP esperando sem_reanudar_pcp");
            sem_wait(&sem_reanudar_pcp);
        }

        log_info(debug_logger, "Se recibio el pcb:");
        pcb_debug_print(pcb_recibido);

        // Por defecto, enviar un nuevo proceso en la proxima iteracion.
        planificar_nuevo_proceso = true;

        switch (motivo) {
        case FIN_QUANTUM:
            manejar_fin_quantum(pcb_recibido);
            break;
        case FIN_PROCESO:
            manejar_fin_proceso(pcb_recibido, params->conexion_memoria);
            break;
        case WAIT_RECURSO:
            manejar_wait_recurso(pcb_recibido, params->conexion_cpu_dispatch, params->conexion_memoria);
            break;
        case SIGNAL_RECURSO:
            manejar_signal_recurso(pcb_recibido, params->conexion_cpu_dispatch, params->conexion_memoria);
            break;
        case IO:
            manejar_io(pcb_recibido);
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
        // TODO actualizar proceso_desalojo_previamente en algun lado segun corresponda.
        if (!proceso_desalojo_previamente) {
            // Desalojar por fin de Quantum
            desalojar_proceso(conexion_cpu_interrupt);
        }
    }
}

void desalojar_proceso(int conexion_interrupt)
{
    // TODO enviar un mensaje al CPU para desalojar
    assert(false && "No implementado");
}

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb **pcb_actualizado)
{
    t_list *elementos = recibir_paquete(conexion_cpu_dispatch);
    *pcb_actualizado = list_get(elementos, 0);
    t_motivo_desalojo *motivo = list_get(elementos, 1);
    t_motivo_desalojo motivo_ret = *motivo;

    free(motivo);
    list_destroy(elementos);

    return motivo_ret;
}

void manejar_fin_quantum(t_pcb *pcb_recibido)
{
    // Lo volvemos a agregar a la cola de READY
    squeue_push(cola_ready, pcb_recibido);

    // Logs
    log_info(kernel_logger, "PID: %d - Desalojado por fin de Quantum", pcb_recibido->pid);
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", pcb_recibido->pid);
    char *lista_pids = obtener_lista_pids(cola_ready);
    log_info(kernel_logger, "Cola Ready cola_ready: [%s]", lista_pids);
    free(lista_pids);
}

void manejar_fin_proceso(t_pcb *pcb_recibido, int conexion_memoria)
{
    eliminar_proceso(pcb_recibido, "SUCCESS", conexion_memoria);
}

void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria)
{
    // El CPU, luego de hacer wait, envia el nombre del recurso en un mensaje.
    char *nombre_recurso = recibir_mensaje(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido, conexion_memoria);
        return;
    }

    // NOTE No necesito mutexear esto ya que los recursos solo son modificados por el pcp.
    int *cant_recurso = sdictionary_get(instancias_recursos, nombre_recurso);

    log_info(debug_logger,
             "PID: %d - Esperando recurso %s, valor: %d",
             pcb_recibido->pid,
             nombre_recurso,
             *cant_recurso);

    // Si quedan instancias del recurso.
    if (*cant_recurso > 0) {
        // Asignarle el recurso y enviarlo al CPU.
        asignar_recurso(pcb_recibido->pid, nombre_recurso);

        pcb_send(pcb_recibido, socket_conexion_dispatch);
        planificar_nuevo_proceso = false; // No enviar otro proceso durante la proxima iteracion.
    } else {
        // Agregarlo a la cola de bloqueados
        t_squeue *cola = sdictionary_get(colas_blocked_recursos, nombre_recurso);
        squeue_push(cola, pcb_recibido);
    }

    // Reducir el contador de instancias del recurso.
    (*cant_recurso)--;
}

void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria)
{
    // El CPU, luego de hacer signal, envia el nombre del recurso en un mensaje.
    char *nombre_recurso = recibir_mensaje(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido, conexion_memoria);
        return;
    }

    // Eliminar la asignacion
    liberar_asignacion_recurso(pcb_recibido->pid, nombre_recurso);

    // NOTE No necesito mutexear esto ya que los recursos solo son modificados por el pcp.
    int *cant_recurso = sdictionary_get(colas_blocked_recursos, nombre_recurso);

    // Si es <0, hay procesos bloqueados.
    if (*cant_recurso < 0) {
        // Sacar el primer proceso de la cola de bloqueados.
        t_squeue *cola = sdictionary_get(instancias_recursos, nombre_recurso);
        t_pcb *pcb = squeue_pop(cola);

        // Asignarle el recurso y agregarlo a ready.
        asignar_recurso(pcb->pid, nombre_recurso);

        squeue_push(cola_ready, pcb);
    }

    // Incrementar el contador de instancias del recurso.
    (*cant_recurso)++;

    // Devolver la ejecucion al proceso.
    pcb_send(pcb_recibido, socket_conexion_dispatch);
    planificar_nuevo_proceso = false; // No enviar otro proceso durante la proxima iteracion.
}

void manejar_io(t_pcb *pcb_recibido)
{
    // TODO
    assert(false && "No implementado");
}

void recurso_invalido(t_pcb *pcb_recibido, int conexion_memoria)
{
    eliminar_proceso(pcb_recibido, "INVALID_RESOURCE", conexion_memoria);
}

void asignar_recurso(uint32_t pid, char *recurso)
{
    t_list *asignaciones = sdictionary_get(asignaciones_recursos, recurso);
    uint32_t *elem = malloc(sizeof(uint32_t));
    *elem = pid;
    list_add(asignaciones, elem);
}

void liberar_asignacion_recurso(uint32_t pid, char *recurso)
{
    t_list *asignaciones = sdictionary_get(asignaciones_recursos, recurso);

    // Iterar hasta encontrar el pid y eliminarlo.
    // NOTE si no hay asignaciones no se hace nada, esto es correcto.
    t_list_iterator *it = list_iterator_create(asignaciones);
    while (list_iterator_has_next(it)) {
        uint32_t *pid_asignado = list_iterator_next(it);
        if (*pid_asignado == pid) {
            list_iterator_remove(it);
            free(pid_asignado);
            break;
        }
    }
    list_iterator_destroy(it);
}
