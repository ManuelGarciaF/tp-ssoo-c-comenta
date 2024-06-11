#include "main.h"

void manejar_stdin(int conexion_kernel, int conexion_memoria)
{
    while (true) {
        t_list *paquete = recibir_paquete(conexion_kernel);
        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        size_t *tamanio_total = list_get(paquete, 2);

        if (*instruccion != STDIN_READ) {
            log_error(debug_logger, "Operacion invalida");
            list_destroy_and_destroy_elements(paquete, free);
            continue;
        }

        log_info(entradasalida_logger, "PID: %u - Operacion: IO_STDIN_READ", *pid);

        char *buffer = readline("> ");
        size_t buffer_offset = 0;  // Para rastrear el offset en el buffer
        
        t_list_iterator *it = list_iterator_create(paquete);
        list_iterator_next(it);
        list_iterator_next(it);
        list_iterator_next(it);

        while (list_iterator_has_next(it)) {
            t_bloque *bloque = list_iterator_next(it);

            enviar_int(OPCODE_ESCRITURA_ESPACIO_USUARIO, conexion_memoria);

            t_paquete *p = crear_paquete();
            agregar_a_paquete(p, pid, sizeof(uint32_t));
            agregar_a_paquete(p, &(bloque->base), sizeof(size_t));
            agregar_a_paquete(p, &(bloque->tamanio), sizeof(size_t));
            
            if (buffer_offset + bloque->tamanio > *tamanio_total) {
                log_error(debug_logger, "El tamaño del bloque excede el tamaño total del buffer");
                abort();
            }

            char *sub_string = strndup(buffer + buffer_offset, bloque->tamanio);
            agregar_a_paquete(p, sub_string, bloque->tamanio);
            buffer_offset += bloque->tamanio;
            free(sub_string);

            enviar_paquete(p, conexion_memoria);
            
            eliminar_paquete(p);
        }
        
        
        list_iterator_destroy(it);

        free(buffer);
            
        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos

        list_destroy_and_destroy_elements(paquete, free);
    }
}