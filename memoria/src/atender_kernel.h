#ifndef ATENDER_KERNEL_H_
#define ATENDER_KERNEL_H_
#include "main.h"

/*
** Definiciones de funciones
*/

void recibir_crear_proceso(int socket_conexion);
void recibir_liberar_proceso(int socket_conexion);
t_list *devolver_lineas(char *path);
void eliminar_pseudocodigo(void *data);

#endif // ATENDER_KERNEL_H_
