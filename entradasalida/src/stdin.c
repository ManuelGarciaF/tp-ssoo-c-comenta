#include "main.h"
#include "utils/bloque.h"

static void stdin_read(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_memoria);

void manejar_stdin(int conexion_kernel, int conexion_memoria)
{

    while (true) {
        bool err = false;
        t_list *paquete = recibir_paquete(conexion_kernel, &err);
        if (err) {
            log_error(debug_logger, "Hubo un error en la conexion con kernel");
            abort();
        }

        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        size_t *tamanio_total = list_get(paquete, 2);

        if (*instruccion != STDIN_READ) {
            log_error(debug_logger, "Operacion invalida");
            list_destroy_and_destroy_elements(paquete, free);
            continue;
        }

        log_info(entradasalida_logger, "PID: %u - Operacion: IO_STDIN_READ", *pid);

        stdin_read(paquete, *pid, *tamanio_total, conexion_memoria);
        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos

        list_destroy_and_destroy_elements(paquete, free);
    }
}

static void stdin_read(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_memoria)
{
    char *buffer = readline("> ");
    size_t buffer_offset = 0; // Para rastrear el offset en el buffer

    t_list_iterator *it = list_iterator_create(paquete);
    list_iterator_next(it);
    list_iterator_next(it);
    list_iterator_next(it);

    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);

        if (buffer_offset + bloque->tamanio > tamanio_total) {
            log_error(debug_logger, "El tamaño del bloque excede el tamaño total del buffer");
            abort();
        }

        char *sub_string = strndup(buffer + buffer_offset, bloque->tamanio);
        escribir_bloque_a_memoria(pid, *bloque, sub_string, conexion_memoria);
        free(sub_string);

        buffer_offset += bloque->tamanio;
    }

    list_iterator_destroy(it);

    free(buffer);
}
