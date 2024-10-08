#include "main.h"
#include "utils/slist.h"
#include <commons/log.h>

// Definiciones locales
static void recibir_crear_proceso(int socket_conexion);
static void recibir_liberar_proceso(int socket_conexion);
static t_list *devolver_lineas(char *path_relativo);

void atender_kernel(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (kernel)");
    while (true) {
        bool err = false;
        t_op_memoria op = recibir_int(socket_conexion, &err);
        if (err) {
            log_error(debug_logger, "Hubo un error en la conexión con el kernel");
            abort();
        }
        log_info(debug_logger, "Operacion enviada por el kernel: %d", op);

        // Esperar antes de responder.
        usleep(RETARDO_RESPUESTA * 1000);

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

        // Avisar que se termino la operacion
        enviar_int(MENSAJE_OP_TERMINADA, socket_conexion);
    }
}

static void recibir_crear_proceso(int socket_conexion)
{
    bool err = false;
    t_list *paquete = recibir_paquete(socket_conexion, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexion con el kernel");
        abort();
    }
    uint32_t *pid = list_get(paquete, 0);
    char *pid_str = string_itoa(*pid);
    char *path_relativo = list_get(paquete, 1);

    log_info(debug_logger, "Cargando instrucciones para PID: %d, con path: %s", *pid, path_relativo);

    // Cargamos las instrucciones a partir del path
    t_list *lineas = devolver_lineas(path_relativo);

    t_proceso *proceso_nuevo = proceso_create(lineas);

    // Guardamos el proceso en el diccionario
    sdictionary_put(procesos, pid_str, proceso_nuevo);

    log_info(memoria_logger, "PID: %u - Tamaño: 0", *pid);

    free(pid_str);
    list_destroy_and_destroy_elements(paquete, free);
}

static void recibir_liberar_proceso(int socket_conexion)
{
    // Recibir el pid
    bool err = false;
    uint32_t pid = recibir_int(socket_conexion, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexion con el kernel");
        abort();
    }
    log_info(debug_logger, "Liberando proceso con pid: %u", pid);
    char *pid_str = string_itoa(pid);

    t_proceso *proceso = sdictionary_remove(procesos, pid_str);

    log_info(memoria_logger, "PID: %u - Tamaño: %d", pid, slist_size(proceso->paginas));

    // Marcar marcos como libres.
    pthread_mutex_lock(&mutex_bitmap_marcos);
    slist_lock(proceso->paginas);
    t_list_iterator *it = list_iterator_create(proceso->paginas->list);
    while (list_iterator_has_next(it)) {
        int *num_marco = list_iterator_next(it);
        bitarray_clean_bit(bitmap_marcos, *num_marco);
    }
    list_iterator_destroy(it);
    slist_unlock(proceso->paginas);
    pthread_mutex_unlock(&mutex_bitmap_marcos);

    // Liberar el t_proceso
    proceso_destroy(proceso);

    free(pid_str);
}

static t_list *devolver_lineas(char *path_relativo)
{
    char *path_completo = string_new();
    string_append(&path_completo, PATH_INSTRUCCIONES);
    string_append(&path_completo, "/"); // Ya que es un directorio.
    string_append(&path_completo, path_relativo);

    FILE *archivo = fopen(path_completo, "r");
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
    free(linea); // Siempre alojamos una linea de mas

    fclose(archivo);
    free(path_completo);
    return lineas;
}
