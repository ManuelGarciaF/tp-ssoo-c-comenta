#include "main.h"

// Definiciones locales
static void enviar_instruccion(int socket_conexion);
static void responder_acceso_tabla_paginas(int socket_conexion);

void atender_cpu(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (cpu)");

    // Enviar el tamanio de página
    enviar_int(tam_pagina, socket_conexion);

    while (true) {
        t_op_memoria op = recibir_int(socket_conexion);

        // Esperar antes de responder.
        usleep(retardo_respuesta * 1000);

        switch (op) {
        case OPCODE_SOLICITAR_INSTRUCCION:
            enviar_instruccion(socket_conexion);
            break;
        case OPCODE_ACCESO_TABLA_PAGINAS:
            responder_acceso_tabla_paginas(socket_conexion);
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

    t_proceso *proceso = sdictionary_get(procesos, pid);

    if ((int) *pc >= list_size(proceso->codigo)) {
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

    if ((int) *num_pagina >= slist_size(proceso->paginas)) {
        log_error(debug_logger, "PID %u: La página a la que se intento acceder (%u) no existe", *pid_int, *num_pagina);
        abort();
    }

    uint32_t *num_marco = slist_get(proceso->paginas, *num_pagina);

    // Enviar el numero de marco.
    enviar_int(*num_marco, socket_conexion);

    free(pid);
}
