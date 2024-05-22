#ifndef INTERFACES_H_
#define INTERFACES_H_

#include "main.h"

t_interfaz *registrar_interfaz(int conexion_io);

int enviar_operacion(t_bloqueado_io *pb, int conexion_io);

int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io);

#endif // INTERFACES_H_
