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
t_log *debug_logger;
t_log *kernel_logger;

// Variables de config
char *puerto_escucha;
char *ip_memoria;
char *puerto_memoria;
char *ip_cpu;
char *puerto_cpu_dispatch;
char *puerto_cpu_interrupt;

/*
** Definiciones de funciones
*/

void cargar_config(void);

void atender_io(int *socket_conexion);

#endif // MAIN_H_
