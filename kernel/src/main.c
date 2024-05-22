#include "./main.h"

/*
** Variables Globales
*/
t_log *debug_logger;
t_log *kernel_logger;
t_config *config;

// Variables de config
char *puerto_escucha;
char *ip_memoria;
char *puerto_memoria;
char *ip_cpu;
char *puerto_cpu_dispatch;
char *puerto_cpu_interrupt;
int grado_multiprogramacion;
int quantum;
t_algoritmo_planificacion algoritmo_planificacion;

uint32_t pid_en_ejecucion = -1; // -1 Cuando no hay ninguno
int procesos_extra_multiprogramacion = 0;

t_squeue *cola_new;
t_squeue *cola_ready;
t_sdictionary *colas_blocked_recursos;

t_sdictionary *interfaces_conectadas;

t_sdictionary *instancias_recursos;
t_slist *asignaciones_recursos;

// Semaforos
sem_t sem_multiprogramacion;
sem_t sem_elementos_en_new;
sem_t sem_elementos_en_ready;
sem_t sem_interrupciones_activadas;

bool planificacion_pausada = false;
sem_t sem_reanudar_pcp;
sem_t sem_reanudar_plp;

int main(int argc, char *argv[])
{
    inicializar_globales();

    // Conexion con el cpu
    int conexion_cpu_dispatch = crear_conexion(ip_cpu, puerto_cpu_dispatch);
    if (!realizar_handshake(conexion_cpu_dispatch)) {
        log_error(debug_logger, "No Se pudo realizar un handshake con el CPU (dispatch)");
    }

    int conexion_cpu_interrupt = crear_conexion(ip_cpu, puerto_cpu_interrupt);
    if (!realizar_handshake(conexion_cpu_interrupt)) {
        log_error(debug_logger, "No se pudo realizar un handshake con el CPU (interrupt)");
    }

    // Conexion con la memoria
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    if (!realizar_handshake(conexion_memoria)) {
        log_error(debug_logger, "No se pudo realizar un handshake con la memoria");
    }
    // Avisar quien es a la memoria
    enviar_str(MENSAJE_A_MEMORIA_KERNEL, conexion_memoria);

    // Crear hilo para esperar conexiones de entrada/salida
    pthread_t hilo_esperar_io;
    int iret = pthread_create(&hilo_esperar_io, NULL, esperar_conexiones_io, NULL);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para esperar conexiones");
    }

    pthread_t hilo_planificador_largo_plazo;
    iret = pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo, &conexion_memoria);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
    }

    pthread_t hilo_planificador_corto_plazo;
    t_parametros_pcp params_pcp = {conexion_cpu_dispatch, conexion_cpu_interrupt, conexion_memoria};
    iret = pthread_create(&hilo_planificador_corto_plazo, NULL, planificador_corto_plazo, &params_pcp);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el planificador de corto plazo");
    }

    correr_consola();
    // Cuando vuelve esta función, el programa debe terminar.

    // Cerrar sockets
    close(conexion_cpu_dispatch);
    close(conexion_cpu_interrupt);
    close(conexion_memoria);

    liberar_globales();

    pthread_exit(NULL);
}

void inicializar_globales(void)
{
    debug_logger = log_create("kernel_debug.log", "kernel_debug", true, LOG_LEVEL_INFO);
    kernel_logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);

    config = config_create("kernel.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    puerto_escucha = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_or_exit(config, "IP_CPU");
    puerto_cpu_dispatch = config_get_string_or_exit(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_or_exit(config, "PUERTO_CPU_INTERRUPT");
    grado_multiprogramacion = config_get_int_or_exit(config, "GRADO_MULTIPROGRAMACION");
    algoritmo_planificacion =
        parse_algoritmo_planifiacion(config_get_string_or_exit(config, "ALGORITMO_PLANIFICACION"));
    quantum = config_get_int_or_exit(config, "QUANTUM");

    // sem_multiprogramacion es el numero de procesos nuevos que se pueden crear,
    // comienza en el grado de multiprogramacion.
    sem_init(&sem_multiprogramacion, 0, grado_multiprogramacion);
    // sem_elementos_en_new permite bloquear el plp hasta que se lo necesita,
    // contiene el numero de elementos en cola_new.
    sem_init(&sem_elementos_en_new, 0, 0);

    // Contiene el numero de elementos en cola_ready.
    sem_init(&sem_elementos_en_ready, 0, 0);

    cola_new = squeue_create();
    cola_ready = squeue_create();
    colas_blocked_recursos = sdictionary_create();

    interfaces_conectadas = sdictionary_create();

    instancias_recursos = sdictionary_create();
    asignaciones_recursos = slist_create();

    char **recursos_strs = config_get_array_value(config, "RECURSOS");
    char **instancias_recursos_strs = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    for (int i = 0; recursos_strs[i] != NULL; i++) {
        // Guardar los recursos disponibles.
        int *instancias = malloc(sizeof(int));
        *instancias = atoi(instancias_recursos_strs[i]);
        sdictionary_put(instancias_recursos, recursos_strs[i], instancias);

        // Inicializar las colas de bloqueados para cada recurso.
        t_squeue *cola_bloqueados = squeue_create();
        sdictionary_put(colas_blocked_recursos, recursos_strs[i], cola_bloqueados);
    }

    string_array_destroy(recursos_strs);
    string_array_destroy(instancias_recursos_strs);

    sem_init(&sem_reanudar_pcp, 0, 0);
    sem_init(&sem_reanudar_plp, 0, 0);
}

void liberar_globales(void)
{
    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(kernel_logger);

    config_destroy(config);

    // NOTE todos estos destroy no liberan la memoria de los elementos que contienen,
    // no es importante ya que el programa termina.
    squeue_destroy(cola_new);
    squeue_destroy(cola_ready);
    sdictionary_destroy(colas_blocked_recursos);
    sdictionary_destroy(interfaces_conectadas);

    sdictionary_destroy(instancias_recursos);
    slist_destroy(asignaciones_recursos);

    sem_destroy(&sem_multiprogramacion);
    sem_destroy(&sem_elementos_en_new);
    sem_destroy(&sem_elementos_en_ready);

    sem_destroy(&sem_reanudar_pcp);
    sem_destroy(&sem_reanudar_plp);
}

void *esperar_conexiones_io(void *param)
{
    // Esperar constantemente conexiones de io
    int socket_escucha = iniciar_servidor(puerto_escucha);
    while (true) {
        // Guardo el socket en el heap para no perderlo
        int *conexion_io = malloc(sizeof(int));
        *conexion_io = esperar_cliente(socket_escucha);

        // Crear hilo para manejar esta conexion
        pthread_t hilo;
        int iret = pthread_create(&hilo, NULL, atender_io, conexion_io);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para atender la interfaz de I/O");
            exit(1);
        }
        pthread_detach(hilo);
    }
}

t_algoritmo_planificacion parse_algoritmo_planifiacion(char *str)
{
    if (!strcmp(str, "FIFO")) {
        return FIFO;
    } else if (!strcmp(str, "RR")) {
        return RR;
    } else if (!strcmp(str, "VRR")) {
        return VRR;
    }
    log_error(debug_logger, "El algoritmo de planificacion en el config no es valido");
    exit(1);
}

void pausar_planificacion()
{
    // Asegurarse que los semaforos esten en 0
    sem_trywait(&sem_reanudar_pcp);
    sem_trywait(&sem_reanudar_plp);

    planificacion_pausada = true;
}

void reanudar_planificacion()
{
    if (!planificacion_pausada) {
        log_error(debug_logger, "La planificacion no se encuentra pausada");
    }

    // Hay que ponerlo en falso antes de los signals.
    planificacion_pausada = false;
    sem_post(&sem_reanudar_pcp);
    sem_post(&sem_reanudar_plp);
}

void eliminar_proceso(t_pcb *pcb, int conexion_memoria)
{
    // Si se achico el grado_multiprogramacion, los proximos procesos que
    // se eliminen no liberan espacio en el sem_multiprogramacion.
    if (procesos_extra_multiprogramacion > 0) {
        procesos_extra_multiprogramacion--;
    } else {
        sem_post(&sem_multiprogramacion);
    }

    // Avisar a memoria que debe liberar el proceso.
    enviar_str(MENSAJE_FIN_PROCESO, conexion_memoria);
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(pcb->pid));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // Liberar todos los recursos en base a asignaciones_recursos.
    slist_lock(asignaciones_recursos);
    t_list_iterator *it = list_iterator_create(asignaciones_recursos->list);
    while (list_iterator_has_next(it)) {
        t_asignacion_recurso *ar = list_iterator_next(it);
        if (ar->pid == pcb->pid) {
            list_iterator_remove(it);            // Eliminarlo de la lista
            liberar_recurso(ar->nombre_recurso); // Liberar ese recurso

            // Liberar ar
            free(ar->nombre_recurso);
            free(ar);
        }
    }
    list_iterator_destroy(it);
    slist_unlock(asignaciones_recursos);

    pcb_destroy(pcb);
}
