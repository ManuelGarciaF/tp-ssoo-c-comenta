#ifndef MAIN_H_
#define MAIN_H_

#include <commons/log.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/sockets.h>
#include <utils/utils.h>
#include <string.h>
#include <utils/mensajes.h>
#include <commons/collections/list.h>

#define LIMITE_LINEA_INSTRUCCION (sizeof(char) * 255)

/*
** Estructuras
*/

/*
** Variables globales
*/

extern t_log *debug_logger;
extern t_log *memoria_logger;
extern t_dictionary *codigo_procesos;

// Variables de config
extern char *puerto_escucha;


/*
** Definiciones de funciones
*/

void cargar_config(t_config *config);
void atender_conexion(int *socket_conexion);
void atender_io(int socket_conexion);
void atender_kernel(int socket_conexion);
void atender_cpu(int socket_conexion);

#endif // MAIN_H_
