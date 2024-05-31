#include "main.h"

// Definiciones locales
static void enviar_instruccion(int socket_conexion);

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
            // Esperar el tiempo establecido por el config
            enviar_instruccion(socket_conexion);
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

    int *pid_int = list_get(info_fetch, 0);
    char *pid = string_itoa(*pid_int);
    int *pc = list_get(info_fetch, 1);

    t_proceso *proceso = dictionary_get(procesos, pid);

    if (*pc >= list_size(proceso->codigo)) {
        log_error(debug_logger, "El PC: %d supera la cantidad de instrucciones del PID %s", *pc, pid);
        abort();
    }

    char *instruccion = list_get(proceso->codigo, *pc);
    enviar_str(instruccion, socket_conexion);

    log_info(debug_logger, "Se envio la instruccion \"%s\", al PID: %d, con PC: %d", instruccion, *pid_int, *pc);

    list_destroy_and_destroy_elements(info_fetch, free);
    free(pid);
}
