#include "main.h"

static void stdout_write(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_kernel,
                         int conexion_memoria);

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

        stdout_write(paquete, *pid, *tamanio_total, conexion_kernel, conexion_memoria);
        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos

        list_destroy_and_destroy_elements(paquete, free);
    }
}

static void stdout_write(t_list *paquete, uint32_t pid, size_t tamanio_total, int conexion_kernel, int conexion_memoria)
{

    t_list_iterator *it = list_iterator_create(paquete);
    list_iterator_next(it);
    list_iterator_next(it);
    list_iterator_next(it);

    char *buffer = malloc(tamanio_total + 1);
    assert(buffer != NULL);
    size_t buffer_length = 0;

    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);

        enviar_int(OPCODE_LECTURA_ESPACIO_USUARIO, conexion_memoria);

        t_paquete *p = crear_paquete();
        agregar_a_paquete(p, &pid, sizeof(uint32_t));
        agregar_a_paquete(p, &(bloque->base), sizeof(size_t));
        agregar_a_paquete(p, &(bloque->tamanio), sizeof(size_t));
        enviar_paquete(p, conexion_memoria);
        eliminar_paquete(p);

        bool err = false;
        t_list *paquete_respuesta = recibir_paquete(conexion_memoria, &err);
        if (err) {
            log_error(debug_logger, "Hubo un error en la conexion con memoria");
        }

        char *respuesta = list_get(paquete_respuesta, 0);

        memcpy(buffer + buffer_length, respuesta, bloque->tamanio);
        buffer_length += bloque->tamanio;

        list_destroy_and_destroy_elements(paquete_respuesta, free);
    }

    buffer[buffer_length] = '\0';
    printf("%s\n", buffer);
    free(buffer);

    list_iterator_destroy(it);
}
