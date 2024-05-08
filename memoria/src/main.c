#include "./main.h"

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

    // diccionario con pseudocodigo de procesos
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
}

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

void atender_cpu(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (cpu)");
}

void atender_kernel(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (kernel)");
    while (true) {
        char *mensaje = recibir_mensaje(socket_conexion);
        if (strcmp(mensaje, MENSAJE_INICIO_PROCESO) == 0) {
            recibir_crear_proceso(socket_conexion);
        } else if (strcmp(mensaje, MENSAJE_FIN_PROCESO) == 0) {
            recibir_liberar_proceso(socket_conexion);
        }

        free(mensaje);
    }
}

void atender_io(int socket_conexion)
{
    log_info(debug_logger, "Se Se conecto correctamente (io)");
}

void recibir_crear_proceso(int socket_conexion)
{
    t_list *paquete = recibir_paquete(socket_conexion);
    uint32_t *pid = list_get(paquete, 0);
    char *pid_str = string_itoa(*pid);
    char *path = list_get(paquete, 1);

    log_info(debug_logger, "Cargando instrucciones para PID: %d, con path: %s", *pid, path);

    t_list *lineas = devolver_lineas(path);
    dictionary_put(codigo_procesos, pid_str, lineas);

    free(pid_str);
    list_destroy_and_destroy_elements(paquete, free);
}

void recibir_liberar_proceso(int socket_conexion)
{
    t_list *paquete = recibir_paquete(socket_conexion);
    uint32_t *pid = list_get(paquete, 0);
    char *pid_str = string_itoa(*pid);
    dictionary_remove_and_destroy(codigo_procesos, pid_str, (void *)eliminar_data);
}

t_list *devolver_lineas(char *path)
{
    FILE *archivo = fopen(path, "r");
    // Verifica si el archivo se abrió correctamente
    if (archivo == NULL) {
        log_error(debug_logger, "El archivo no se pudo abrir correctamente.");
        return NULL;
    }

    t_list *lineas = list_create();
    char *linea = malloc(LIMITE_LINEA_INSTRUCCION);
    while (fgets(linea, LIMITE_LINEA_INSTRUCCION, archivo) != NULL) {
        list_add(lineas, linea);
        // Allojar espacio para nuevas lineas
        linea = malloc(LIMITE_LINEA_INSTRUCCION);
    }
    // FIXME solo para debuggear
    /* t_list_iterator *iterator = list_iterator_create(lineas); */
    /* printf("Se leyo archivo de path %s\n", path); */
    /* while (list_iterator_has_next(iterator)) { */
    /*     printf("%s", (char *)list_iterator_next(iterator)); */
    /* } */
    /* list_iterator_destroy(iterator); */

    free(linea);
    fclose(archivo);
    return lineas;
}

void eliminar_data(t_list *data)
{
    list_destroy_and_destroy_elements(data, free);
}
