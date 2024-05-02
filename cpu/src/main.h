#ifndef MAIN_H_
#define MAIN_H_

#include <commons/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/sockets.h>
#include <utils/utils.h>
#include <utils/mensajes.h>
#include <estructuras/pcb.h>
#include "decoder.h"

/*
** Variables globales
*/
extern t_log *debug_logger;
extern t_log *cpu_logger;
extern t_pcb *pcb;

// Variables de config
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *puerto_escucha_dispatch;
extern char *puerto_escucha_interrupt;

/*
** Definiciones de funciones
*/
void cargar_config(t_config *config);

void *servidor_dispatch(int *socket_escucha);
void *servidor_interrupt(int *socket_escucha);
int aceptar_conexion_kernel(int socket_escucha);
char *fetch(uint32_t pid, uint32_t program_counter, int conexion_memoria);

#endif // MAIN_H_
