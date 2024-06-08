#include "./main.h"

void atender_io(int socket_conexion)
{
    log_info(debug_logger, "Se conecto correctamente (io)");

    while (true) {
        t_op_memoria op = recibir_int(socket_conexion);

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