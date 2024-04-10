#ifndef MAIN_H_
#define MAIN_H_

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
t_log *cpu_logger;

// Variables de config
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;

/*
** Definiciones de funciones
*/
void cargar_config(void);

void *servidor_dispatch(int *socket_escucha);
void *servidor_interrupt(int *socket_escucha);

#endif // MAIN_H_
