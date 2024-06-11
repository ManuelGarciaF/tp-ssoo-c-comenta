#include "main.h"

// Estructuras
typedef struct {
    int64_t ms_espera;
    uint32_t pid;
    bool cancelado;
} t_parametros_reloj_rr;

// Variables globales
static bool planificar_nuevo_proceso;
static bool *cancelar_ultimo_reloj;    // Permite cancelar el ultimo reloj.
static bool en_ejecucion_ultimo_reloj; // Permite saber si el ultimo reloj ya termino.
static pthread_mutex_t mutex_en_ejecucion_ultimo_reloj;

// Definiciones locales
static void *reloj_rr(void *param);
static t_motivo_desalojo recibir_pcb(t_pcb **pcb_actualizado);
static t_pcb *peek_siguiente_pcb(bool *rplus);
static void pop_siguiente_pcb(void);

// Motivos de devolucion de pcb
static void manejar_fin_quantum(t_pcb *pcb_recibido);
static void manejar_fin_proceso(t_pcb *pcb_recibido);
static void manejar_out_of_memory(t_pcb *pcb_recibido);
static void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch);
static void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch);
static void manejar_io(t_pcb *pcb_recibido, int conexion_dispatch);
static void manejar_interrumpido_por_usuario(t_pcb *pcb_recibido);

static void recurso_invalido(t_pcb *pcb_recibido);
static void interfaz_invalida(t_pcb *pcb_recibido);

// Pasa procesos de READY a EXEC
void *planificador_corto_plazo(void *vparams)
{
    pthread_t hilo_reloj_rr;
    planificar_nuevo_proceso = true;
    t_temporal *cronometro = NULL;

    pthread_mutex_init(&mutex_en_ejecucion_ultimo_reloj, NULL);

    while (true) {
        // No siempre hay que enviar un proceso nuevo segun el algoritmo de planificacion,
        // a veces devolvemos la ejecucion al proceso devuelto.
        if (planificar_nuevo_proceso) {
            bool proceso_ready_plus;
            // Esperar que haya elementos en ready.
            sem_wait(&sem_elementos_en_ready);
            t_pcb *pcb_inicial = peek_siguiente_pcb(&proceso_ready_plus);

            // Tomar el permiso para agregar procesos a exec
            sem_wait(&sem_entrada_a_exec);

            // Ver que mientras esperabamos no nos hayan sacado el proceso en ready (eliminado por consola)
            t_pcb *pcb_a_ejecutar = peek_siguiente_pcb(&proceso_ready_plus);
            // Si cambio o no tenemos ningun proceso, no hacer nada
            if (pcb_inicial != pcb_a_ejecutar || pcb_inicial == NULL || pcb_a_ejecutar == NULL) {
                // Liberar el permiso para agregar procesos a exec
                sem_post(&sem_entrada_a_exec);
                continue;
            }

            // Sacar el pcb que vimos antes de la cola
            pop_siguiente_pcb();

            log_info(kernel_logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb_a_ejecutar->pid);

            // Si estamos en RR o VRR, iniciar el reloj.
            // NOTE Solo iniciamos un reloj nuevo cuando se planifica un proceso nuevo, si un proceso volvio a
            // ejecutarse (syscall), seguimos con el mismo.
            if (ALGORITMO_PLANIFICACION == RR || ALGORITMO_PLANIFICACION == VRR) {

                if (ALGORITMO_PLANIFICACION == VRR) {
                    // Reiniciar el quantum del proceso si no lo sacamos de ready_plus,
                    // o si su quantum restante es menor a 0.
                    if (!proceso_ready_plus || pcb_a_ejecutar->quantum <= 0) {
                        pcb_a_ejecutar->quantum = QUANTUM;
                    }

                    // (Re)iniciar el cronometro.
                    if (cronometro != NULL) {
                        temporal_destroy(cronometro);
                    }
                    cronometro = temporal_create();
                }

                log_info(debug_logger,
                         "Iniciando reloj (V)RR con q=%ld para pid=%u",
                         pcb_a_ejecutar->quantum,
                         pcb_a_ejecutar->pid);

                t_parametros_reloj_rr *params_reloj = malloc(sizeof(t_parametros_reloj_rr));
                params_reloj->ms_espera = pcb_a_ejecutar->quantum;
                params_reloj->pid = pcb_a_ejecutar->pid;
                params_reloj->cancelado = false;

                cancelar_ultimo_reloj = &(params_reloj->cancelado); // Guardo la ubicacion del campo cancelado
                en_ejecucion_ultimo_reloj = true;

                int iret = pthread_create(&hilo_reloj_rr, NULL, reloj_rr, params_reloj);
                if (iret != 0) {
                    log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
                }
            }

            // Enviamos el pcb a CPU.
            pcb_send(pcb_a_ejecutar, conexion_cpu_dispatch);
            pid_en_ejecucion = pcb_a_ejecutar->pid; // Registrar que proceso esta en ejecucion

            // Liberar el permiso para agregar procesos a exec
            sem_post(&sem_entrada_a_exec);

            pcb_destroy(pcb_a_ejecutar);
        }

        // En este punto, se esta ejecutando el proceso, esperamos que interrumpa y
        // nos devuelvan el pcb actualizado y el motivo.

        t_pcb *pcb_recibido = NULL;
        t_motivo_desalojo motivo = recibir_pcb(&pcb_recibido); // Bloqueante
        pid_en_ejecucion = -1;
        assert(pcb_recibido != NULL);

        if (ALGORITMO_PLANIFICACION == VRR) {
            // Frenar el cronometro y reducir el quantum del proceso
            int64_t tiempo_transcurrido_ms = temporal_gettime(cronometro);
            pcb_recibido->quantum -= tiempo_transcurrido_ms;
            log_info(debug_logger, "Pasaron %ldms, Q restante: %ldms", tiempo_transcurrido_ms, pcb_recibido->quantum);
        }

        // Despues de recibir el pcb, cancelar el hilo del reloj, en caso de que siga corriendo.
        pthread_mutex_lock(&mutex_en_ejecucion_ultimo_reloj);
        if (en_ejecucion_ultimo_reloj) {
            // Significa que la estructura de parametros no fue liberada, podemos pedirle que cancele
            // sin segfaultear.
            *cancelar_ultimo_reloj = true;
            en_ejecucion_ultimo_reloj = false;
            log_info(debug_logger, "Se recibio un pcb y el reloj seguia en ejecucion, pidiendole que cancele...");
        }
        pthread_mutex_unlock(&mutex_en_ejecucion_ultimo_reloj);

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
        case OUT_OF_MEMORY:
            manejar_out_of_memory(pcb_recibido);
            break;
        case WAIT_RECURSO:
            manejar_wait_recurso(pcb_recibido, conexion_cpu_dispatch);
            break;
        case SIGNAL_RECURSO:
            manejar_signal_recurso(pcb_recibido, conexion_cpu_dispatch);
            break;
        case IO:
            manejar_io(pcb_recibido, conexion_cpu_dispatch);
            break;
        case INTERRUMPIDO_POR_USUARIO:
            manejar_interrumpido_por_usuario(pcb_recibido);
            break;
        }

        // Devolver el permiso para manejar el desalojo.
        sem_post(&sem_manejo_desalojo_cpu);
    }
}

static t_pcb *peek_siguiente_pcb(bool *rplus)
{
    // El primer elemento de ready+, sino el primer elemento de ready, sino NULL

    if (!squeue_is_empty(cola_ready_plus)) {
        *rplus = true;
        return squeue_peek(cola_ready_plus);
    }

    if (!squeue_is_empty(cola_ready)) {
        *rplus = false;
        return squeue_peek(cola_ready);
    }

    return NULL;
}

static void pop_siguiente_pcb(void)
{
    // Hace pop al elemento que vimos con peek_siguiente_pcb
    if (!squeue_is_empty(cola_ready_plus)) {
        squeue_pop(cola_ready_plus);
        return;
    }

    if (!squeue_is_empty(cola_ready)) {
        squeue_pop(cola_ready);
        return;
    }
}

static void *reloj_rr(void *param)
{
    int64_t ms_espera = ((t_parametros_reloj_rr *)param)->ms_espera;
    uint32_t pid = ((t_parametros_reloj_rr *)param)->pid;
    assert(ms_espera >= 0);

    // Esperar
    usleep(ms_espera * 1000);

    // Solo enviar la interrupcion si este reloj no fue cancelado mientras esperaba
    bool cancelar_reloj = ((t_parametros_reloj_rr *)param)->cancelado;
    if (!cancelar_reloj) {
        log_info(debug_logger,
                 "Reloj para pid=%u termino despues de esperar %ldms, enviando interrupcion",
                 pid,
                 ms_espera);

        // Ver si tenemos permiso para enviar interrupciones
        // (bonus: sirve como mutex para la conexion de interrupt)
        sem_wait(&sem_interrupcion_rr);

        // Desalojar el proceso
        t_paquete *paquete = crear_paquete();
        agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
        t_motivo_desalojo motivo = FIN_QUANTUM;
        agregar_a_paquete(paquete, &motivo, sizeof(t_motivo_desalojo));
        enviar_paquete(paquete, conexion_cpu_interrupt);
        eliminar_paquete(paquete);

        // Volver a habilitar el envio de interrupciones
        sem_post(&sem_interrupcion_rr);

        // Avisar que este reloj termino su ejecucion,
        // si fue cancelado, en_ejecucion_ultimo reloj es setteado a falso cuando se cancela
        pthread_mutex_lock(&mutex_en_ejecucion_ultimo_reloj);
        en_ejecucion_ultimo_reloj = false;
        pthread_mutex_unlock(&mutex_en_ejecucion_ultimo_reloj);
    } else {
        log_info(debug_logger, "El reloj para pid=%u fue cancelado", pid);
    }

    free(param);

    return NULL;
}

static t_motivo_desalojo recibir_pcb(t_pcb **pcb_actualizado)
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

    // Logs
    log_info(kernel_logger, "PID: %d - Desalojado por fin de Quantum", pcb_recibido->pid);
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", pcb_recibido->pid);
    log_cola_ready();

    sem_post(&sem_elementos_en_ready);
}

static void manejar_fin_proceso(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: SUCCESS", pcb_recibido->pid);

    eliminar_proceso(pcb_recibido);
}

static void manejar_out_of_memory(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", pcb_recibido->pid);

    eliminar_proceso(pcb_recibido);
}

static void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch)
{
    // El CPU,sasa luego de hacer wait, envia el nombre del recurso.
    char *nombre_recurso = recibir_str(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT.
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido);
        return;
    }

    bool proceso_bloquea = asignar_recurso(pcb_recibido, nombre_recurso);

    if (proceso_bloquea) {

        // Agregarlo a la cola de bloqueados del recurso.
        t_squeue *cola = sdictionary_get(colas_blocked_recursos, nombre_recurso);
        squeue_push(cola, pcb_recibido);

        log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", pcb_recibido->pid);
        log_info(kernel_logger, "PID: %d - Bloqueado por: %s", pcb_recibido->pid, nombre_recurso);

        planificar_nuevo_proceso = true;

    } else { // El proceso no bloqueo.

        // Devolver la ejecucion al proceso.
        pcb_send(pcb_recibido, socket_conexion_dispatch);
        free(pcb_recibido);
        planificar_nuevo_proceso = false; // No enviar otro proceso durante la proxima iteracion.
    }

    free(nombre_recurso);
}

static void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch)
{
    // El CPU, luego de hacer signal, envia el nombre del recurso.
    char *nombre_recurso = recibir_str(socket_conexion_dispatch);

    // Si el recurso no existe, enviarlo a EXIT
    if (!sdictionary_has_key(instancias_recursos, nombre_recurso)) {
        recurso_invalido(pcb_recibido);
        return;
    }

    liberar_recurso(pcb_recibido->pid, nombre_recurso);

    // Devolver la ejecucion al proceso.
    pcb_send(pcb_recibido, socket_conexion_dispatch);
    free(pcb_recibido);
    planificar_nuevo_proceso = false; // No enviar otro proceso durante la proxima iteracion.

    free(nombre_recurso);
}

static void manejar_io(t_pcb *pcb_recibido, int conexion_dispatch)
{
    // Recibir los parametros enviados por la cpu luego de desalojar
    t_list *operacion = recibir_paquete(conexion_dispatch);
    char *nombre_interfaz = list_get(operacion, 0);
    t_operacion_io *opcode = list_get(operacion, 1);

    // Si no existe la interfaz, o no soporta la operacion, mandar el proceso a EXIT.
    if (!existe_interfaz(nombre_interfaz) || !interfaz_soporta_operacion(nombre_interfaz, *opcode)) {
        interfaz_invalida(pcb_recibido);
        list_destroy_and_destroy_elements(operacion, free);
        return;
    }

    // Agregarlo a la cola de bloqueados de la interfaz.
    t_bloqueado_io *tbi = malloc(sizeof(t_bloqueado_io));
    tbi->pcb = pcb_recibido;
    tbi->opcode = *opcode;
    tbi->operacion = operacion;

    t_interfaz *interfaz = sdictionary_get(interfaces_conectadas, nombre_interfaz);
    squeue_push(interfaz->bloqueados, tbi);

    log_info(kernel_logger, "PID: %d - Bloqueado por: %s", pcb_recibido->pid, nombre_interfaz);

    // Avisar a la interfaz que hay un proceso nuevo esperando.
    sem_post(&(interfaz->procesos_esperando));
}

static void manejar_interrumpido_por_usuario(t_pcb *pcb_recibido)
{
    log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pcb_recibido->pid);
    log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", pcb_recibido->pid);
    eliminar_proceso(pcb_recibido);
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
