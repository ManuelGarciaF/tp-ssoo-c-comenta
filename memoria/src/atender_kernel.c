#include "atender_kernel.h"

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
        // Quitar el \n final leido por fgets
        if (linea[strlen(linea) - 1] == '\n') {
            linea[strlen(linea) - 1] = '\0';
        }

        list_add(lineas, linea);

        // Allojar espacio para nuevas lineas
        linea = malloc(LIMITE_LINEA_INSTRUCCION);
    }
    // FIXME solo para debuggear
    // t_list_iterator *iterator = list_iterator_create(lineas); 
    // printf("Se leyo archivo de path %s\n", path); 
    // while (list_iterator_has_next(iterator)) { 
    //     printf("%s", (char *)list_iterator_next(iterator)); 
    // } 
    // list_iterator_destroy(iterator); 

    free(linea);
    fclose(archivo);
    return lineas;
}

void eliminar_data(t_list *data)
{
    list_destroy_and_destroy_elements(data, free);
}
