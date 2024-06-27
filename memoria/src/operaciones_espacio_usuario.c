#include "main.h"

static bool direccion_asignada_a_proceso(char *pid, size_t dir_inicio);

void responder_lectura_espacio_usuario(int socket_conexion)
{
    bool err = false;
    t_list *params = recibir_paquete(socket_conexion, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexion");
        abort();
    }

    uint32_t *pid_int = list_get(params, 0);
    char *pid = string_itoa(*pid_int);
    size_t *dir_inicio = list_get(params, 1);
    size_t *tam_leer = list_get(params, 2);

    if (!direccion_asignada_a_proceso(pid, *dir_inicio)) {
        log_error(debug_logger, "El proceso %u intento acceder una dirección que no tiene asignada", *pid);
        abort();
    }

    // Ver que no nos salimos de la pagina
    int offset_inicio = *dir_inicio % TAM_PAGINA;
    int offset_fin = offset_inicio + *tam_leer; // No inclusivo
    if (offset_fin > TAM_PAGINA) {
        log_error(debug_logger,
                  "El proceso %u intento leer afuera de la pagina, DF: %zu, tam: %zu",
                  *pid,
                  *dir_inicio,
                  *tam_leer);
        abort();
    }

    // Copiamos la memoria a leer al paquete
    void *inicio_memoria_a_leer = (char *)memoria_de_usuario + *dir_inicio;

    t_paquete *p = crear_paquete();

    // Copiar tam_leer bytes desde inicio_memoria_a_leer al paquete
    pthread_mutex_lock(&mutex_memoria_de_usuario);
    agregar_a_paquete(p, inicio_memoria_a_leer, *tam_leer);
    pthread_mutex_unlock(&mutex_memoria_de_usuario);

    enviar_paquete(p, socket_conexion);
    eliminar_paquete(p);

    log_info(memoria_logger,
             "PID: %u - Accion: LEER - Direccion fisica: %zu - Tamaño %zu",
             *pid_int,
             *dir_inicio,
             *tam_leer);

    list_destroy_and_destroy_elements(params, free);
    free(pid);
}

void responder_escritura_espacio_usuario(int socket_conexion)
{
    bool err = false;
    t_list *params = recibir_paquete(socket_conexion, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexion");
        abort();
    }

    uint32_t *pid_int = list_get(params, 0);
    char *pid = string_itoa(*pid_int);
    size_t *dir_inicio = list_get(params, 1);
    size_t *tam_a_escribir = list_get(params, 2);
    void *datos_a_escribir = list_get(params, 3);

    if (!direccion_asignada_a_proceso(pid, *dir_inicio)) {
        log_error(debug_logger, "El proceso %u intento acceder una dirección que no tiene asignada", *pid);
        abort();
    }

    // Ver que no nos salimos de la pagina
    int offset_inicio = *dir_inicio % TAM_PAGINA;
    int offset_fin = offset_inicio + *tam_a_escribir; // No inclusivo
    if (offset_fin > TAM_PAGINA) {
        log_error(debug_logger,
                  "El proceso %u intento escribir afuera de la pagina, DF: %zu, tam: %zu",
                  *pid,
                  *dir_inicio,
                  *tam_a_escribir);
        abort();
    }

    // Copiar la memoria recibida a la memoria de usuario.
    void *inicio_memoria_a_escribir = (char *)memoria_de_usuario + *dir_inicio;
    pthread_mutex_lock(&mutex_memoria_de_usuario);
    memcpy(inicio_memoria_a_escribir, datos_a_escribir, *tam_a_escribir);
    pthread_mutex_unlock(&mutex_memoria_de_usuario);

    log_info(memoria_logger,
             "PID: %u - Accion: ESCRIBIR - Direccion fisica: %zu - Tamaño %zu",
             *pid_int,
             *dir_inicio,
             *tam_a_escribir);

    list_destroy_and_destroy_elements(params, free);
    free(pid);

    enviar_int(MENSAJE_FIN_ESCRITURA, socket_conexion);
}

static bool direccion_asignada_a_proceso(char *pid, size_t dir_inicio)
{
    int num_marco = dir_inicio / TAM_PAGINA;

    t_proceso *proceso = sdictionary_get(procesos, pid);

    bool encontrado = false;

    slist_lock(proceso->paginas);
    t_list_iterator *it = list_iterator_create(proceso->paginas->list);
    while (list_iterator_has_next(it)) {
        int *marco_asignado = list_iterator_next(it);
        if (*marco_asignado == num_marco) {
            encontrado = true;
            break;
        }
    }
    list_iterator_destroy(it);
    slist_unlock(proceso->paginas);

    return encontrado;
}
