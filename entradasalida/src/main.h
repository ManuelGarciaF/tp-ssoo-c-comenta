#ifndef MAIN_H_
#define MAIN_H_

#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/sockets.h>
#include <utils/utils.h>

/*
** Variables globales
*/
typedef enum {
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS
} t_tipo_interfaz;

t_log *debug_logger;
t_log *entradasalida_logger;

// Variables de config
t_tipo_interfaz tipo_interfaz;
char *tiempo_unidad_trabajo;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
char *path_base_dialfs;
char *block_size;
char *block_count;

/*
** Definiciones de funciones
*/

void cargar_config(char* ruta);

#endif // MAIN_H_