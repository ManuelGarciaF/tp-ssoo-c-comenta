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

t_squeue *cola_new;
t_squeue *cola_ready;
t_dictionary *colas_blocked;

// Semaforos
sem_t sem_multiprogramacion;
sem_t sem_elementos_en_new;
sem_t sem_elementos_en_ready;
sem_t sem_proceso_en_ejecucion;

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
    enviar_mensaje(MENSAJE_A_MEMORIA_KERNEL, conexion_memoria);

    // Crear hilo para esperar conexiones de entrada/salida
    pthread_t hilo_esperar_io;
    int iret = pthread_create(&hilo_esperar_io, NULL, (void *)esperar_conexiones_io, NULL);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para esperar conexiones");
    }

    pthread_t hilo_planificador_largo_plazo;
    iret = pthread_create(&hilo_planificador_largo_plazo, NULL, (void *)planificador_largo_plazo, &conexion_memoria);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
    }

    pthread_t hilo_planificador_corto_plazo;
    iret = pthread_create(&hilo_planificador_corto_plazo, NULL, (void *)planificador_corto_plazo, &conexion_cpu_dispatch);
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

    // Inicia en 1, ya que se puede ejecutar un proceso a la vez.
    sem_init(&sem_proceso_en_ejecucion, 0, 1);

    cola_new = squeue_create();
    cola_ready = squeue_create();
    colas_blocked = NULL; // TODO ver esto
}

void liberar_globales(void)
{
    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(kernel_logger);

    config_destroy(config);

    squeue_destroy(cola_new);
    squeue_destroy(cola_ready);
    // TODO liberar colas_blocked cuando este implementada.

    sem_destroy(&sem_multiprogramacion);
    sem_destroy(&sem_elementos_en_new);
    sem_destroy(&sem_elementos_en_ready);
    sem_destroy(&sem_proceso_en_ejecucion);
}

void esperar_conexiones_io(void)
{
    // Esperar constantemente conexiones de io
    int socket_escucha = iniciar_servidor(puerto_escucha);
    while (true) {
        // Guardo el socket en el heap para no perderlo
        int *conexion_io = malloc(sizeof(int));
        *conexion_io = esperar_cliente(socket_escucha);

        // Crear hilo para manejar esta conexion
        pthread_t hilo;
        int iret = pthread_create(&hilo, NULL, (void *)atender_io, conexion_io);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para atender la interfaz de I/O");
            exit(1);
        }
        pthread_detach(hilo);
    }
}

/* Maneja las conexiones de los dispositivos de I/O */
void atender_io(int *socket_conexion)
{
    recibir_handshake(*socket_conexion);

    close(*socket_conexion);
    free(socket_conexion);
    pthread_exit(NULL);
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
