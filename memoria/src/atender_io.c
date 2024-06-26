#include "./main.h"
#include "utils/slist.h"
#include <commons/log.h>

void atender_io(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (io)");

    while (true) {
        bool err = false;
        t_op_memoria op = recibir_int(socket_conexion, &err);
        if (err) {
            log_warning(debug_logger, "La conexion con la interfaz fue cerrada");
            return;
        }

        // Esperar antes de responder.
        usleep(RETARDO_RESPUESTA * 1000);

        switch (op) {
        case OPCODE_LECTURA_ESPACIO_USUARIO:
            responder_lectura_espacio_usuario(socket_conexion);
            break;
        case OPCODE_ESCRITURA_ESPACIO_USUARIO:
            responder_escritura_espacio_usuario(socket_conexion);
            break;
        default:
            log_error(debug_logger, "IO envio una operacion invalida");
            abort();
        }
    }
}
