#include "./main.h"

int main(int argc, char *argv[])
{
    debug_logger =
        log_create("kernel_debug.log", "kernel_debug", true, LOG_LEVEL_INFO);
    kernel_logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);

    cargar_config();

    // Conexion con el cpu
    int conexion_cpu_dispatch = crear_conexion(ip_cpu, puerto_cpu_dispatch);

    if (realizar_handshake(conexion_cpu_dispatch)) {
        log_info(debug_logger,
                 "Se pudo realizar un handshake con el CPU (dispatch)");
    }

    int conexion_cpu_interrupt = crear_conexion(ip_cpu, puerto_cpu_interrupt);
    if (realizar_handshake(conexion_cpu_interrupt)) {
        log_info(debug_logger,
                 "Se pudo realizar un handshake con el CPU (interrupt)");
    }

    // Conexion con la memoria
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    if (realizar_handshake(conexion_memoria)) {
        log_info(debug_logger, "Se pudo realizar un handshake con la memoria");
    }

    // TODO iniciar hilo para la consola del kernel

    // Conexiones con I/O
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

        // TODO Cuando romper este loop
    }

    // Cerrar sockets
    close(conexion_cpu_dispatch);
    close(conexion_cpu_interrupt);
    close(conexion_memoria);

    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(kernel_logger);

    // No matar los hilos al terminar el programa
    pthread_exit(NULL);
    return 0;
}

/* Inicializa las variables globales */
void cargar_config()
{
    t_config *config = config_create("kernel.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    puerto_escucha = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_or_exit(config, "IP_CPU");
    puerto_cpu_dispatch =
        config_get_string_or_exit(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt =
        config_get_string_or_exit(config, "PUERTO_CPU_INTERRUPT");

    config_destroy(config);
}

/* Maneja las conexiones de los dispositivos de I/O */
void atender_io(int *socket_conexion)
{
    recibir_handshake(*socket_conexion);

    close(*socket_conexion);
    free(socket_conexion);
    pthread_exit(NULL);
}
