#include "bloque.h"

void *leer_bloque_de_memoria(uint32_t pid, t_bloque bloque, int conexion_memoria)
{
    enviar_int(OPCODE_LECTURA_ESPACIO_USUARIO, conexion_memoria);

    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, &pid, sizeof(uint32_t));
    agregar_a_paquete(p, &(bloque.base), sizeof(size_t));
    agregar_a_paquete(p, &(bloque.tamanio), sizeof(size_t));
    int bytes = enviar_paquete(p, conexion_memoria);
    assert(bytes > 0);
    eliminar_paquete(p);

    bool err = false;
    t_list *paquete_respuesta = recibir_paquete(conexion_memoria, &err);
    if (err) {
        log_error(debug_logger,
                  "Hubo un error en la conexion con memoria al intentar leer un bloque (PID: %u, base: %zu, tam: %zu)",
                  pid,
                  bloque.base,
                  bloque.tamanio);
        return NULL;
    }

    void *respuesta = list_get(paquete_respuesta, 0);
    list_destroy(paquete_respuesta);

    return respuesta;
}

void escribir_bloque_a_memoria(uint32_t pid, t_bloque bloque, void *data, int conexion_memoria)
{
    enviar_int(OPCODE_ESCRITURA_ESPACIO_USUARIO, conexion_memoria);

    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, &pid, sizeof(uint32_t));
    agregar_a_paquete(p, &(bloque.base), sizeof(size_t));
    agregar_a_paquete(p, &(bloque.tamanio), sizeof(size_t));
    agregar_a_paquete(p, data, bloque.tamanio);
    int bytes = enviar_paquete(p, conexion_memoria);
    assert(bytes > 0);
    eliminar_paquete(p);

    bool err = false;
    if (recibir_int(conexion_memoria, &err) != MENSAJE_FIN_ESCRITURA || err) {
        log_error(debug_logger, "Memoria no devolvio MENSAJE_FIN_ESCRITURA o hubo un error en la conexion");
        abort();
    }
}
