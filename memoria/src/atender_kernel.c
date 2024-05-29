#include "main.h"

// Definiciones locales
static void recibir_crear_proceso(int socket_conexion);
static void recibir_liberar_proceso(int socket_conexion);
static t_list *devolver_lineas(char *path);
static void eliminar_pseudocodigo(void *data);

void atender_kernel(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (kernel)");
    while (true) {
        t_op_memoria op = recibir_int(socket_conexion);
        log_info(debug_logger, "Operacion enviada por el kernel: %d", op);

        switch (op) {
        case OPCODE_INICIO_PROCESO:
            recibir_crear_proceso(socket_conexion);
            break;
        case OPCODE_FIN_PROCESO:
            recibir_liberar_proceso(socket_conexion);
            break;
        default:
            log_error(debug_logger, "Kernel envio una operación invalida");
            abort();
        }
    }
}

static void recibir_crear_proceso(int socket_conexion)
{
    t_list *paquete = recibir_paquete(socket_conexion);
    uint32_t *pid = list_get(paquete, 0);
    char *pid_str = string_itoa(*pid);
    char *path_relativo = list_get(paquete, 1);

    char *path_completo = string_new();
    string_append(&path_completo, path_instrucciones);
    string_append(&path_completo, "/"); // Ya que es un directorio.
    string_append(&path_completo, path_relativo);

    log_info(debug_logger, "Cargando instrucciones para PID: %d, con path: %s", *pid, path_completo);

    t_list *lineas = devolver_lineas(path_completo);
    free(path_completo);

    dictionary_put(codigo_procesos, pid_str, lineas);

    free(pid_str);
    list_destroy_and_destroy_elements(paquete, free);
}

static void recibir_liberar_proceso(int socket_conexion)
{
    t_list *paquete = recibir_paquete(socket_conexion);
    uint32_t *pid = list_get(paquete, 0);
    log_info(debug_logger, "Liberando proceso con pid: %u", *pid);
    char *pid_str = string_itoa(*pid);
    dictionary_remove_and_destroy(codigo_procesos, pid_str, eliminar_pseudocodigo);

    free(pid_str);
    list_destroy_and_destroy_elements(paquete, free);
}

static t_list *devolver_lineas(char *path)
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

    free(linea);
    fclose(archivo);
    return lineas;
}

static void eliminar_pseudocodigo(void *data)
{
    list_destroy_and_destroy_elements((t_list *)data, free);
}
