#ifndef MAIN_H_
#define MAIN_H_

#include "utilidades.h"
#include <assert.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <estructuras/pcb.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/mensajes.h>
#include <utils/sdictionary.h>
#include <utils/sockets.h>
#include <utils/squeue.h>
#include <utils/utils.h>

/*
** Estructuras
*/

typedef struct {
    uint32_t pid;
    char *path;
} t_proceso_nuevo;

typedef struct {
    int conexion_cpu_dispatch;
    int conexion_cpu_interrupt;
    int conexion_memoria;
} t_parametros_pcp;

typedef enum { FIFO, RR, VRR } t_algoritmo_planificacion;

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
extern t_algoritmo_planificacion algoritmo_planificacion;
extern int grado_multiprogramacion;
extern int quantum;

// Colas de procesos
extern t_squeue *cola_new;                      // Contiene t_proceso_nuevo
extern t_squeue *cola_ready;                    // Contiene t_pcb
extern t_sdictionary *colas_blocked_recursos;   // Contiene squeues de t_pcb
extern t_sdictionary *colas_blocked_interfases; // Contiene squeues de t_pcb

// Recursos
extern t_sdictionary *instancias_recursos;
extern t_sdictionary *asignaciones_recursos; // Necesito saber que procesos tienen que recursos para liberarlos
                                             // al eliminar al proceso. Contiene listas de pids de procesos que
                                             // retienen cada recurso.

// Semaforos
extern sem_t sem_multiprogramacion; // Semaforo de espacio restante de multiprogamacion
extern sem_t sem_elementos_en_new;
extern sem_t sem_elementos_en_ready;
/* extern sem_t sem_proceso_en_ejecucion; */ // No estoy seguro si es necesario, esperamos que el proceso termine en el
                                             // bloqueando en el recv del planificador a corto plazo
extern sem_t sem_interrupciones_activadas;

/*
** Definiciones de funciones
*/

// Inicializa y libera las variables globales.
void inicializar_globales(void);
void liberar_globales(void);

void esperar_conexiones_io(void);

void correr_consola(void);

void atender_io(int *socket_conexion);

t_algoritmo_planificacion parse_algoritmo_planifiacion(char *str);

void planificador_largo_plazo(int *conexion_memoria);
void planificador_corto_plazo(t_parametros_pcp *params);

// Solicita a memoria eliminar un proceso y libera sus recursos.
void eliminar_proceso(t_pcb *pcb, int conexion_memoria);

#endif // MAIN_H_
