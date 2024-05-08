#include "atender_cpu.h"

void atender_cpu(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (cpu)");
    while (true) {
        char *mensaje = recibir_mensaje(socket_conexion);
        log_info(debug_logger, "Se recibio el mensaje: %s",mensaje);
        if (strcmp(mensaje, MENSAJE_SOLICITAR_INSTRUCCION) == 0) {
            
            t_list *info_fetch = recibir_paquete(socket_conexion);
            enviar_instrucciones(info_fetch,socket_conexion);
        }

        free(mensaje);
    }
}

void enviar_instrucciones(t_list *info_fetch, int socket_conexion)
{
    int *pid_aux = list_get(info_fetch, 0);
    char *pid = string_itoa(*pid_aux);
    int *pc = list_get(info_fetch, 1);
    
    t_list *lineas_de_pid = dictionary_get(codigo_procesos, pid);
    char *instruccion = list_get(lineas_de_pid, *pc);
    enviar_mensaje(instruccion, socket_conexion);

    list_destroy_and_destroy_elements(info_fetch, free);
    free(pid);
}