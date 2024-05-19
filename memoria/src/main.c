#include "./main.h"

/*
** VARIABLES GLOBALES
*/
t_log *debug_logger;
t_log *memoria_logger;

char *puerto_escucha;
int retardo_respuesta;

t_dictionary *codigo_procesos;


int main(int argc, char *argv[])
{

    // logs
    debug_logger = log_create("memoria_debug.log", "memoria_debug", true, LOG_LEVEL_INFO);
    memoria_logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);

    // config
    t_config *config = config_create("memoria.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    // Diccionario con pseudocodigo de procesos
    codigo_procesos = dictionary_create();

    cargar_config(config);

    // Esperar conexiones
    int socket_escucha = iniciar_servidor(puerto_escucha);
    while (true) {
        // Guardo el socket en el heap para no perderlo
        int *conexion = malloc(sizeof(int));
        *conexion = esperar_cliente(socket_escucha);

        // Crear hilo para manejar esta conexion
        pthread_t hilo;
        int iret = pthread_create(&hilo, NULL, (void *)atender_conexion, conexion);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para atender la conexion");
            exit(1);
        }
        pthread_detach(hilo);
    }

    return 0;
}

/* Inicializa las variables globales */
void cargar_config(t_config *config)
{
    puerto_escucha = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    retardo_respuesta = config_get_int_or_exit(config, "RETARDO_RESPUESTA");
}

/*
** ATENDER CONEXIONES
*/
void atender_conexion(int *socket_conexion)
{
    recibir_handshake(*socket_conexion);

    char *modulo = recibir_mensaje(*socket_conexion);

    if (strcmp(modulo, MENSAJE_A_MEMORIA_KERNEL) == 0) {
        free(modulo);
        atender_kernel(*socket_conexion);
    } else if (strcmp(modulo, MENSAJE_A_MEMORIA_CPU) == 0) {
        free(modulo);
        atender_cpu(*socket_conexion);
    } else if (strcmp(modulo, MENSAJE_A_MEMORIA_IO) == 0) {
        free(modulo);
        atender_io(*socket_conexion);
    } else {
        log_error(debug_logger, "El cliente no informo su identidad.");
        pthread_exit(NULL);
    }

    close(*socket_conexion);
    free(socket_conexion);
    pthread_exit(NULL);
}

// Atender conexion io
void atender_io(int socket_conexion)
{
    log_info(debug_logger, "Se Se conecto correctamente (io)");
}
