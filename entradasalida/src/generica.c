#include "generica.h"

void generica(int conexion_kernel)
{
    while (true) {
        t_list *paquete = recibir_paquete(conexion_kernel);
        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        uint32_t *unidades_de_trabajo = list_get(paquete, 2);

        if (*instruccion == GEN_SLEEP) {
            log_info(entradasalida_logger, "PID: %u - Operacion: IO_GEN_SLEEP", *pid);

            int milisegundos_de_espera = *unidades_de_trabajo * TIEMPO_UNIDAD_TRABAJO;
            usleep(milisegundos_de_espera * 1000); // Multiplicado por 1000 ya que toma microsegundos.

            enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos
        } else {
            log_error(debug_logger, "Operacion invalida");
        }

        list_destroy_and_destroy_elements(paquete, free);
    }
}
