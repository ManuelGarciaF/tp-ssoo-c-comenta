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
extern t_tipo_interfaz tipo_interfaz;
extern char *nombre_interfaz;
extern int tiempo_unidad_trabajo;
extern char *ip_kernel;
extern char *puerto_kernel;
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *path_base_dialfs;
extern int block_size;
extern int block_count;

/*
** Definiciones de funciones
*/

void cargar_config(t_config *config);
t_tipo_interfaz parsear_a_t_tipo_interfaz(char* str);
void generica(int conexion_kernel);

#endif // MAIN_H_
