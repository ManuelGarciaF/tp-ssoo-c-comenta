#include "./main.h"

int main(int argc, char *argv[])
{
    debug_logger =
        log_create("cpu_debug.log", "cpu_debug", true, LOG_LEVEL_INFO);
    cpu_logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);

    t_config *config = config_create("cpu.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }
    cargar_config(config);

    int socket_escucha_dispatch = iniciar_servidor(puerto_escucha_dispatch);
    int socket_escucha_interrupt = iniciar_servidor(puerto_escucha_interrupt);

    pthread_t hilo_dispatch, hilo_interrupt;
    // Crear hilo para dispatch
    if (pthread_create(&hilo_dispatch,
                       NULL,
                       (void *)servidor_dispatch,
                       &socket_escucha_dispatch) != 0) {
        log_error(debug_logger,
                  "No se pudo crear un hilo para el servidor de dispatch");
        exit(1);
    }

    // Crear hilo para interrupt
    if (pthread_create(&hilo_interrupt,
                       NULL,
                       (void *)servidor_interrupt,
                       &socket_escucha_interrupt) != 0) {
        log_error(debug_logger,
                  "No se pudo crear un hilo para el servidor de interrupt");
        exit(1);
    }

    // Conectar con la memoria
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);

    if (!realizar_handshake(conexion_memoria)) {
        log_error(debug_logger, "Se pudo realizar un handshake con memoria");
    }

    // Esperar que los hilos terminen
    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    // Cerrar sockets
    /* liberar_conexion(conexion_memoria); */

    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(cpu_logger);
    config_destroy(config);

    // Esperar
    return 0;
}

/* Inicializa las variables globales */
void cargar_config(t_config *config)
{
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch =
        config_get_string_or_exit(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt =
        config_get_string_or_exit(config, "PUERTO_ESCUCHA_INTERRUPT");
}

void *servidor_dispatch(int *socket_escucha)
{
    int socket_conexion = esperar_cliente(*socket_escucha);
    if (recibir_handshake(socket_conexion)) {
        log_info(debug_logger,
                 "Se realizo el handshake correctamente (dispatch)");
    }

    close(*socket_escucha);
    return NULL;
}

void *servidor_interrupt(int *socket_escucha)
{
    int socket_conexion = esperar_cliente(*socket_escucha);
    if (recibir_handshake(socket_conexion)) {
        log_info(debug_logger,
                 "Se realizo el handshake correctamente (interrupt)");
    }

    close(*socket_escucha);
    return NULL;
}
