#include "main.h"
#include "utils/bloque.h"

static void stdout_write(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_memoria);

void manejar_stdout(int conexion_kernel, int conexion_memoria)
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

        if (*instruccion != STDOUT_WRITE) { // Jamas deberia ocurrir
            log_error(debug_logger, "Operacion invalida");
            list_destroy_and_destroy_elements(paquete, free);
            continue;
        }

        log_info(entradasalida_logger, "PID: %u - Operacion: IO_STDOUT_WRITE", *pid);

        stdout_write(paquete, *pid, *tamanio_total, conexion_memoria);
        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos

        list_destroy_and_destroy_elements(paquete, free);
    }
}

static void stdout_write(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_memoria)
{
    t_list_iterator *it = list_iterator_create(paquete);
    list_iterator_next(it);
    list_iterator_next(it);
    list_iterator_next(it);

    char *buffer = malloc(tamanio_total + 1);
    if (buffer == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    size_t buffer_length = 0;

    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);

        char *respuesta = leer_bloque_de_memoria(pid, *bloque, conexion_memoria);
        memcpy(buffer + buffer_length, respuesta, bloque->tamanio);
        free(respuesta);
        buffer_length += bloque->tamanio;
    }

    buffer[buffer_length] = '\0';
    printf("%s\n", buffer);
    free(buffer);

    list_iterator_destroy(it);
}
