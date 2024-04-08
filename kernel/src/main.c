#include "./main.h"

int main(int argc, char *argv[])
{
    debug_logger =
        log_create("kernel_debug.log", "kernel_debug", true, LOG_LEVEL_INFO);
    kernel_logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);

    t_config *config = config_create("kernel.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }
    cargar_config(config);

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
        pthread_create(&hilo, NULL, (void *)atender_io, conexion_io);
        pthread_detach(hilo);

        // TODO Cuando romper este loop
    }

    // Esperar que los hilos terminen.
    pthread_exit(NULL);

    // Cerrar sockets
    close(conexion_cpu_dispatch);
    close(conexion_cpu_interrupt);
    close(conexion_memoria);

    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(kernel_logger);
    config_destroy(config);

    return 0;
}

/* Inicializa las variables globales */
void cargar_config(t_config *config)
{
    puerto_escucha = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_or_exit(config, "IP_CPU");
    puerto_cpu_dispatch =
        config_get_string_or_exit(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt =
        config_get_string_or_exit(config, "PUERTO_CPU_INTERRUPT");
}

char *config_get_string_or_exit(t_config *config, char *key)
{
    if (!config_has_property(config, key)) {
        log_error(debug_logger,
                  "La key %s no existe en el archivo de config",
                  key);
        exit(1);
    }
    return config_get_string_value(config, key);
}

/* Maneja las conexiones de los dispositivos de I/O */
void atender_io(int *socket_conexion)
{
    recibir_handshake(*socket_conexion);

    free(socket_conexion);
    pthread_exit(NULL);
}
