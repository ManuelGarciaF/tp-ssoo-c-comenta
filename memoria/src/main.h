#ifndef MAIN_H_
#define MAIN_H_

#include "estructuras.h"
#include <assert.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/mensajes.h>
#include <utils/sdictionary.h>
#include <utils/sockets.h>
#include <utils/utils.h>

#define LIMITE_LINEA_INSTRUCCION (sizeof(char) * 255)

/*
** Variables globales
*/

extern t_log *debug_logger;
extern t_log *memoria_logger;
extern t_config *config;

// Variables de config
extern char *puerto_escucha;
extern int tam_memoria;
extern int tam_pagina;
extern char *path_instrucciones;
extern int retardo_respuesta;

// Estructuras de memoria
extern t_sdictionary *procesos; // Contiene t_procesos

extern void *memoria_de_usuario;
extern pthread_mutex_t mutex_memoria_de_usuario;

extern int num_marcos;
extern t_bitarray *bitmap_marcos;
extern pthread_mutex_t mutex_bitmap_marcos;

/*
** Funciones compartidas
*/
void atender_io(int socket_conexion);
void atender_kernel(int socket_conexion);
void atender_cpu(int socket_conexion);

void responder_lectura_espacio_usuario(int socket_conexion);
void responder_escritura_espacio_usuario(int socket_conexion);

#endif // MAIN_H_
