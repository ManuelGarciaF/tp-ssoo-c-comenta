#include "main.h"

void manejar_stdout(int conexion_kernel, int conexion_memoria)
{
    while (true) {
        t_list *paquete = recibir_paquete(conexion_kernel);
        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);

        if (*instruccion != STDOUT_WRITE) {
            log_error(debug_logger, "Operacion invalida");
            list_destroy_and_destroy_elements(paquete, free);
            continue;
        }
    
        log_info(entradasalida_logger, "PID: %u - Operacion: IO_STDOUT_WRITE", *pid);

        t_list_iterator *it = list_iterator_create(paquete);
        list_iterator_next(it);
        list_iterator_next(it);

        while(list_iterator_has_next(it)){
            t_bloque *bloque = list_iterator_next(it);

            enviar_int(OPCODE_LECTURA_ESPACIO_USUARIO, conexion_memoria);

            t_paquete *p = crear_paquete();
            agregar_a_paquete(p, pid, sizeof(uint32_t));
            agregar_a_paquete(p, &(bloque->base), sizeof(size_t));
            agregar_a_paquete(p, &(bloque->tamanio), sizeof(size_t));

            enviar_paquete(p, conexion_memoria);
            eliminar_paquete(p);
            t_list *paquete_respuesta = recibir_paquete(conexion_memoria);

            char *respuesta = list_get(paquete_respuesta, 0);
            printf("%s",respuesta);

            list_destroy_and_destroy_elements(paquete_respuesta, free);
        }
        
        printf("\n");
        
        list_iterator_destroy(it);
            
        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos

        list_destroy_and_destroy_elements(paquete, free);
    }
}