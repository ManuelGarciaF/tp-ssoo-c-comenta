#include "atender_cpu.h"

void atender_cpu(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (cpu)");
    while (true) {
        char *mensaje = recibir_str(socket_conexion);
        if (strcmp(mensaje, MENSAJE_SOLICITAR_INSTRUCCION) == 0) {
            t_list *info_fetch = recibir_paquete(socket_conexion);
            // Esperar el tiempo establecido por el config
            usleep(retardo_respuesta * 1000); // Multiplicado por 1000 ya que toma microsegundos.
            enviar_instruccion(info_fetch, socket_conexion);
        } else {
            log_error(debug_logger, "CPU envio una operacion invalida");
            exit(1);
        }

        free(mensaje);
    }
}

void enviar_instruccion(t_list *info_fetch, int socket_conexion)
{
    int *pid_int = list_get(info_fetch, 0);
    char *pid = string_itoa(*pid_int);
    int *pc = list_get(info_fetch, 1);

    t_list *lineas_de_pid = dictionary_get(codigo_procesos, pid);

    if (*pc >= list_size(lineas_de_pid)) {
        log_error(debug_logger, "El PC: %d supera la cantidad de instrucciones del PID %s", *pc, pid);
    }

    char *instruccion = list_get(lineas_de_pid, *pc);
    enviar_str(instruccion, socket_conexion);

    log_info(debug_logger, "Se envio la instruccion \"%s\", al PID: %d, con PC: %d", instruccion, *pid_int, *pc);

    list_destroy_and_destroy_elements(info_fetch, free);
    free(pid);
}
