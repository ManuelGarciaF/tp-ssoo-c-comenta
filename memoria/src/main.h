#ifndef MAIN_H_
#define MAIN_H_

#include <commons/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/sockets.h>
#include <utils/utils.h>
#include <string.h>
#include <utils/mensajes.h>
#include <commons/collections/list.h>

/*
** Estructuras
*/

typedef struct {
    uint32_t pid;
    t_list* lineas;
} t_proceso; 

/*
** Variables globales
*/

t_log *debug_logger;
t_log *memoria_logger;

// Variables de config
char *puerto_escucha;

/*
** Definiciones de funciones
*/

void cargar_config(t_config *config);
void atender_conexion(int *socket_conexion);
void atender_io(int socket_conexion);
void atender_kernel(int socket_conexion);
void atender_cpu(int socket_conexion);
void recibir_crear_proceso(int socket_conexion);
void recibir_liberar_proceso(int socket_conexion);

#endif // MAIN_H_