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
#include <semaphore.h>

/*
** Estructuras
*/

typedef struct {
    uint32_t pid;
    char *path;
} t_proceso_nuevo;

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
extern int grado_multiprogramacion;

// Colas de procesos
extern t_squeue *cola_new; // Contiene t_proceso_nuevo
extern t_squeue *cola_ready; // Contiene t_pcb
extern t_dictionary *colas_blocked; // Contienen t_pcb

// Semaforos
extern sem_t sem_multiprogramacion; // Semaforo de espacio restante de multiprogamacion
extern sem_t sem_elementos_en_new;

/*
** Definiciones de funciones
*/

// Inicializa y libera las variables globales.
void inicializar_globales(void);
void liberar_globales(void);

void esperar_conexiones_io(void);

void correr_consola(void);

void atender_io(int *socket_conexion);

void planificador_largo_plazo(int *conexion_memoria);

#endif // MAIN_H_
