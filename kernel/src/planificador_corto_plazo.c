#include "main.h"

// Variables globales
static bool planificar_nuevo_proceso;
static sem_t sem_comenzar_reloj;

// Estructuras
typedef struct {
    int conexion_cpu_interrupt;
    int ms_espera;
    uint32_t pid;
} t_parametros_reloj_rr;

// Definiciones locales
static void *reloj_rr(void *param);
static t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb **pcb_actualizado);

// Motivos de devolucion de pcb
static void manejar_fin_quantum(t_pcb *pcb_recibido);
static void manejar_fin_proceso(t_pcb *pcb_recibido);
static void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch);
static void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch);
static void manejar_io(t_pcb *pcb_recibido, int conexion_dispatch);
static void manejar_interrumpido_por_usuario(t_pcb *pcb_recibido);

static void recurso_invalido(t_pcb *pcb_recibido);
static void interfaz_invalida(t_pcb *pcb_recibido);

// Pasa procesos de READY a EXEC
void *planificador_corto_plazo(void *vparams)
{
    t_parametros_pcp *params = (t_parametros_pcp *)vparams;

    pthread_t hilo_reloj_rr;
    planificar_nuevo_proceso = true;
    while (true) {
        // No siempre hay que enviar un proceso nuevo segun el algoritmo de planificacion,
        // a veces devolvemos la ejecucion al proceso devuelto.
        if (planificar_nuevo_proceso) {
            // Esperar que haya elementos en ready.
            sem_wait(&sem_elementos_en_ready);

            // Tomar el permiso para agregar procesos a exec
            sem_wait(&sem_entrada_a_exec);
            // TODO checkear que luego de esta espera el proceso en ready por el que se esperaba siga siendo el
            // mismo (puede haber sido eliminado por comando)

            // Por FIFO y RR siempre tomamos el primero
            t_pcb *pcb_a_ejecutar = squeue_pop(cola_ready);
            // Enviamos el pcb a CPU.
            pcb_send(pcb_a_ejecutar, params->conexion_cpu_dispatch);
            pid_en_ejecucion = pcb_a_ejecutar->pid; // Registrar que proceso esta en ejecucion

            // Liberar el permiso para agregar procesos a exec
            sem_post(&sem_entrada_a_exec);

            log_info(kernel_logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb_a_ejecutar->pid);

            // Si estamos en RR o VRR, iniciar el reloj.
            if (algoritmo_planificacion == RR || algoritmo_planificacion == VRR) {
                log_info(debug_logger, "Iniciando reloj RR con q=%d para pid=%u", quantum, pcb_a_ejecutar->pid);

                t_parametros_reloj_rr *params_reloj = malloc(sizeof(t_parametros_reloj_rr));
                params_reloj->conexion_cpu_interrupt = params->conexion_cpu_interrupt;
                params_reloj->ms_espera = quantum;
                params_reloj->pid = pcb_a_ejecutar->pid;

                int iret = pthread_create(&hilo_reloj_rr, NULL, reloj_rr, params_reloj);
                if (iret != 0) {
                    log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
                }
            }

            pcb_destroy(pcb_a_ejecutar);
        }

        // En este punto, se esta ejecutando el proceso, esperamos que interrumpa y
        // nos devuelvan el pcb actualizado y el motivo.

        t_pcb *pcb_recibido = NULL;
        t_motivo_desalojo motivo = recibir_pcb(params->conexion_cpu_dispatch, &pcb_recibido); // Bloqueante
        pid_en_ejecucion = -1;

        log_info(debug_logger, "Se recibio un PCB de CPU con el motivo %d:", motivo);
        pcb_debug_print(pcb_recibido);

        // Esperar si no tenemos permiso para manejar el desalojo.
        // Como gestionamos este permiso no hace falta ver si tenemos permiso para entrar a
        // ready en las funciones de manejar_xxx
        sem_wait(&sem_manejo_desalojo_cpu);

        // Por defecto, enviar un nuevo proceso en la proxima iteracion.
        planificar_nuevo_proceso = true;

        switch (motivo) {
        case FIN_QUANTUM:
            manejar_fin_quantum(pcb_recibido);
            break;
        case FIN_PROCESO:
            manejar_fin_proceso(pcb_recibido);
            break;
        case WAIT_RECURSO:
            manejar_wait_recurso(pcb_recibido, params->conexion_cpu_dispatch);
            break;
        case SIGNAL_RECURSO:
            manejar_signal_recurso(pcb_recibido, params->conexion_cpu_dispatch);
            break;
        case IO:
            manejar_io(pcb_recibido, params->conexion_cpu_dispatch);
            break;
        case INTERRUMPIDO_POR_USUARIO:
            manejar_interrumpido_por_usuario(pcb_recibido);
            break;
        }

        // Devolver el permiso para manejar el desalojo.
        sem_post(&sem_manejo_desalojo_cpu);
    }

    sem_destroy(&sem_comenzar_reloj);
}

static void *reloj_rr(void *param)
{
    int conexion_cpu_interrupt = ((t_parametros_reloj_rr *)param)->conexion_cpu_interrupt;
    int ms_espera = ((t_parametros_reloj_rr *)param)->ms_espera;
    uint32_t pid = ((t_parametros_reloj_rr *)param)->pid;

    // Esperar
    usleep(ms_espera * 1000);

    log_info(debug_logger, "Reloj para pid=%u termino despues de esperar %dms", pid, ms_espera);

    // Desalojar el proceso
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    t_motivo_desalojo motivo = FIN_QUANTUM;
    agregar_a_paquete(paquete, &motivo, sizeof(t_motivo_desalojo));
    enviar_paquete(paquete, conexion_cpu_interrupt);
    eliminar_paquete(paquete);

    free(param);

    return NULL;
}

static t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb **pcb_actualizado)
{
    t_list *elementos = recibir_paquete(conexion_cpu_dispatch);
    *pcb_actualizado = list_get(elementos, 0);
    t_motivo_desalojo *motivo = list_get(elementos, 1);
    t_motivo_desalojo motivo_ret = *motivo;

    free(motivo);
    list_destroy(elementos);

    return motivo_ret;
}

static void manejar_fin_quantum(t_pcb *pcb_recibido)
{
    // Lo volvemos a agregar a la cola de READY
    squeue_push(cola_ready, pcb_recibido);
    sem_post(&sem_elementos_en_ready);

    // Logs
    log_info(kernel_logger, "PID: %d - Desalojado por fin de Quantum", pcb_recibido->pid);
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", pcb_recibido->pid);
    char *lista_pids = obtener_lista_pids_pcb(cola_ready);
    log_info(kernel_logger, "Cola Ready cola_ready: [%s]", lista_pids);
    free(lista_pids);
}

static void manejar_fin_proceso(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: SUCCESS", pcb_recibido->pid);

    eliminar_proceso(pcb_recibido);
}

static void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch)
{
    // El CPU, luego de hacer wait, envia el nombre del recurso en un mensaje.
    char *nombre_recurso = recibir_str(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido);
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

        log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", pcb_recibido->pid);
        log_info(kernel_logger, "PID: %d - Bloqueado por: %s", pcb_recibido->pid, nombre_recurso);
    }

    // Reducir el contador de instancias del recurso.
    (*cant_recurso)--;
}

static void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch)
{
    // El CPU, luego de hacer signal, envia el nombre del recurso en un mensaje.
    char *nombre_recurso = recibir_str(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido);
        return;
    }

    // Eliminar la asignacion
    liberar_asignacion_recurso(pcb_recibido->pid, nombre_recurso);
    liberar_recurso(nombre_recurso);

    // Devolver la ejecucion al proceso.
    pcb_send(pcb_recibido, socket_conexion_dispatch);
    planificar_nuevo_proceso = false; // No enviar otro proceso durante la proxima iteracion.
}

static void manejar_io(t_pcb *pcb_recibido, int conexion_dispatch)
{
    t_list *paquete = recibir_paquete(conexion_dispatch);
    char *nombre_interfaz = list_get(paquete, 0);
    t_operacion_io *operacion = list_get(paquete, 1);

    // Si no existe la interfaz, o no soporta la operacion, mandar el proceso a EXIT.
    if (!existe_interfaz(nombre_interfaz) || !interfaz_soporta_operacion(nombre_interfaz, *operacion)) {
        interfaz_invalida(pcb_recibido);
        return;
    }

    // Antes de agregarlo a la lista de bloqueados, incrementar el pc
    // FIXME esto deberia hacerse en el cpu
    (pcb_recibido->program_counter)++;

    // Agregarlo a la cola de bloqueados de la interfaz.
    t_bloqueado_io *tbi = malloc(sizeof(t_bloqueado_io));
    tbi->pcb = pcb_recibido;
    tbi->opcode = *operacion;
    tbi->operacion = paquete;

    t_interfaz *interfaz = sdictionary_get(interfaces_conectadas, nombre_interfaz);
    squeue_push(interfaz->bloqueados, tbi);

    // Avisar a la interfaz que hay un proceso nuevo esperando.
    sem_post(&(interfaz->procesos_esperando));
}

static void recurso_invalido(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", pcb_recibido->pid);
    eliminar_proceso(pcb_recibido);
}

static void interfaz_invalida(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", pcb_recibido->pid);
    eliminar_proceso(pcb_recibido);
}

static void manejar_interrumpido_por_usuario(t_pcb *pcb_recibido)
{
    // TODO
}
