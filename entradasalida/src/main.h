#ifndef MAIN_H_
#define MAIN_H_

#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/sockets.h>
#include <utils/utils.h>
#include <utils/mensajes.h>

/*
** Variables globales
*/

extern t_log *debug_logger;
extern t_log *entradasalida_logger;

// Variables de config
extern t_tipo_interfaz TIPO_INTERFAZ;
extern char *NOMBRE_INTERFAZ;
extern int TIEMPO_UNIDAD_TRABAJO;
extern char *IP_KERNEL;
extern char *PUERTO_KERNEL;
extern char *IP_MEMORIA;
extern char *PUERTO_MEMORIA;
extern char *PATH_BASE_DIALFS;
extern int BLOCK_SIZE;
extern int BLOCK_COUNT;

/*
** Definiciones de funciones
*/

void cargar_config(t_config *config);
t_tipo_interfaz parsear_a_t_tipo_interfaz(char* str);
void generica(int conexion_kernel);

#endif // MAIN_H_
