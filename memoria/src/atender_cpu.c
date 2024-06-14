#include "main.h"

// Definiciones locales
static void enviar_instruccion(int socket_conexion);
static void responder_acceso_tabla_paginas(int socket_conexion);
static void responder_ajustar_tamanio_proceso(int socket_conexion);
static void ampliar_proceso(t_proceso *proceso, int paginas_a_agregar, int socket_conexion);
static void reducir_proceso(t_proceso *proceso, int paginas_a_liberar, int socket_conexion);

void atender_cpu(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (cpu)");

    // CPU necesita saber el tamanio de pagina para las operaciones de la MMU.
    enviar_int(TAM_PAGINA, socket_conexion);

    while (true) {
        t_op_memoria op = recibir_int(socket_conexion);

        // Esperar antes de responder.
        usleep(RETARDO_RESPUESTA * 1000);

        switch (op) {
        case OPCODE_SOLICITAR_INSTRUCCION:
            enviar_instruccion(socket_conexion);
            break;
        case OPCODE_ACCESO_TABLA_PAGINAS:
            responder_acceso_tabla_paginas(socket_conexion);
            break;
        case OPCODE_AJUSTAR_TAMANIO_PROCESO:
            responder_ajustar_tamanio_proceso(socket_conexion);
            break;
        case OPCODE_LECTURA_ESPACIO_USUARIO:
            responder_lectura_espacio_usuario(socket_conexion);
            break;
        case OPCODE_ESCRITURA_ESPACIO_USUARIO:
            responder_escritura_espacio_usuario(socket_conexion);
            break;
        default:
            log_error(debug_logger, "CPU envio una operacion invalida");
            abort();
        }
    }
}

static void enviar_instruccion(int socket_conexion)
{
    t_list *info_fetch = recibir_paquete(socket_conexion);

    uint32_t *pid_int = list_get(info_fetch, 0);
    char *pid = string_itoa(*pid_int);
    uint32_t *pc = list_get(info_fetch, 1);
    assert(pc != NULL);
    assert(pid_int != NULL);

    t_proceso *proceso = sdictionary_get(procesos, pid);

    if ((int)*pc >= list_size(proceso->codigo)) {
        log_error(debug_logger, "El PC: %u supera la cantidad de instrucciones del PID %s", *pc, pid);
        abort();
    }

    char *instruccion = list_get(proceso->codigo, *pc);
    enviar_str(instruccion, socket_conexion);

    log_info(debug_logger, "Se envio la instruccion \"%s\", al PID: %u, con PC: %u", instruccion, *pid_int, *pc);

    list_destroy_and_destroy_elements(info_fetch, free);
    free(pid);
}

static void responder_acceso_tabla_paginas(int socket_conexion)
{
    // Obtener los parametros de la operacion
    t_list *paquete = recibir_paquete(socket_conexion);

    uint32_t *pid_int = list_get(paquete, 0);
    char *pid = string_itoa(*pid_int);
    uint32_t *num_pagina = list_get(paquete, 1);

    // Buscar proceso
    t_proceso *proceso = sdictionary_get(procesos, pid);

    if ((int)*num_pagina >= slist_size(proceso->paginas)) {
        log_error(debug_logger, "PID %u: La página a la que se intento acceder (%u) no existe", *pid_int, *num_pagina);
        abort();
    }

    uint32_t *num_marco = slist_get(proceso->paginas, *num_pagina);

    log_info(memoria_logger, "PID: %u - Pagina: %u - Marco: %u", *pid_int, *num_pagina, *num_marco);

    // Enviar el numero de marco.
    enviar_int(*num_marco, socket_conexion);

    list_destroy_and_destroy_elements(paquete, free);
    free(pid);
}

static void responder_ajustar_tamanio_proceso(int socket_conexion)
{
    // Obtener los parametros de la operacion
    t_list *paquete = recibir_paquete(socket_conexion);

    uint32_t *pid_int = list_get(paquete, 0);
    char *pid = string_itoa(*pid_int);
    uint32_t *tam_nuevo_bytes = list_get(paquete, 1);

    t_proceso *proceso = sdictionary_get(procesos, pid);
    int paginas_en_uso = slist_size(proceso->paginas);

    int paginas_requeridas_tam_nuevo = ceil_div(*tam_nuevo_bytes, TAM_PAGINA);

    int dif_paginas = paginas_requeridas_tam_nuevo - paginas_en_uso;

    int tam_actual_bytes = paginas_en_uso * TAM_PAGINA;
    if (dif_paginas > 0) {
        log_info(memoria_logger,
                 "PID: %u - Tamaño Actual: %d - Tamaño a Ampliar: %u",
                 *pid_int,
                 tam_actual_bytes,
                 *tam_nuevo_bytes);
        ampliar_proceso(proceso, dif_paginas, socket_conexion);
    } else {
        log_info(memoria_logger,
                 "PID: %u - Tamaño Actual: %d - Tamaño a Reducir: %u",
                 *pid_int,
                 tam_actual_bytes,
                 *tam_nuevo_bytes);
        reducir_proceso(proceso, -dif_paginas, socket_conexion);
    }

    list_destroy_and_destroy_elements(paquete, free);
    free(pid);
}

static void ampliar_proceso(t_proceso *proceso, int paginas_a_agregar, int socket_conexion)
{
    // Ver si hay suficientes marcos libres disponibles y guardar sus indices
    t_list *marcos_a_asignar = list_create();
    pthread_mutex_lock(&mutex_bitmap_marcos);
    // Hasta haber pasado por todo el bitmap o haber obtenido suficientes marcos.
    for (int i = 0; i < num_marcos && list_size(marcos_a_asignar) < paginas_a_agregar; i++) {

        // Si el marco esta libre
        if (bitarray_test_bit(bitmap_marcos, i) == false) {
            /* log_info(debug_logger, "El marco %u estaba libre", i); */
            // Guardamos el numero de marco
            int *marco_libre = malloc(sizeof(int));
            *marco_libre = i;
            list_add(marcos_a_asignar, marco_libre);
        }
    }

    // Si no encontramos suficientes marcos libres avisar OOM
    if (list_size(marcos_a_asignar) < paginas_a_agregar) {
        log_warning(debug_logger, "Fallo la ampliación, Out Of Memory");
        list_clean_and_destroy_elements(marcos_a_asignar, free);
        enviar_int(R_RESIZE_OUT_OF_MEMORY, socket_conexion);
    } else {
        // Asignar marcos y avisar success
        t_list_iterator *it = list_iterator_create(marcos_a_asignar);
        while (list_iterator_has_next(it)) {
            int *num_marco = list_iterator_next(it);

            // Agregarlo a la tabla de paginas
            slist_add(proceso->paginas, num_marco);

            // Marcarlo como ocupado en el bitmap
            bitarray_set_bit(bitmap_marcos, *num_marco);
        }
        list_iterator_destroy(it);

        enviar_int(R_RESIZE_SUCCESS, socket_conexion);
    }

    pthread_mutex_unlock(&mutex_bitmap_marcos);

    // No liberamos los contenidos porque los usamos directamente en la tabla de paginas del proceso.
    list_destroy(marcos_a_asignar);
}

static void reducir_proceso(t_proceso *proceso, int paginas_a_liberar, int socket_conexion)
{
    pthread_mutex_lock(&mutex_bitmap_marcos);

    for (int i = 0; i < paginas_a_liberar; i++) {
        int indice_ultima_pagina = slist_size(proceso->paginas) - 1;
        int *marco_liberado = slist_remove(proceso->paginas, indice_ultima_pagina);

        // Marcar el marco como libre en el bitmap
        bitarray_clean_bit(bitmap_marcos, *marco_liberado);

        free(marco_liberado);
    }

    pthread_mutex_unlock(&mutex_bitmap_marcos);

    enviar_int(R_RESIZE_SUCCESS, socket_conexion);
}
