#ifndef MAIN_H_
#define MAIN_H_

#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <utils/sockets.h>
#include <utils/utils.h>
#include <utils/mensajes.h>
#include <utils/squeue.h>
#include <commons/collections/list.h>
#include <estructuras/pcb.h>
#include <assert.h>

/*
** Estructuras
*/

/*
** Variables globales
*/
extern t_log *debug_logger;
extern t_log *kernel_logger;
extern t_config *config;

// Variables de config
extern char *puerto_escucha;
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *ip_cpu;
extern char *puerto_cpu_dispatch;
extern char *puerto_cpu_interrupt;

// Colas de procesos
extern t_squeue *cola_new;
extern t_squeue *cola_ready;
extern t_dictionary *colas_blocked;

/*
** Definiciones de funciones
*/

// Inicializa y libera las variables globales.
void inicializar_globales(void);
void liberar_globales(void);

void esperar_conexiones_io(void);

void correr_consola(void);

void atender_io(int *socket_conexion);

// Funciones de consola

void iniciar_proceso(char *path);
void finalizar_proceso(char *pid);


#endif // MAIN_H_
