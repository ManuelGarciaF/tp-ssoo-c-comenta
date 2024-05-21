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
#include <utils/slist.h>
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

typedef struct {
    char *nombre_recurso;
    uint32_t pid;
} t_asignacion_recurso;

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

extern uint32_t pid_en_ejecucion;
extern int procesos_extra_multiprogramacion; // Si se achico el grado_multiprogramacion, los proximos procesos que
                                             // se eliminen no liberan espacio en el sem_multiprogramacion.

// Colas de procesos
extern t_squeue *cola_new;                      // Contiene t_proceso_nuevo
extern t_squeue *cola_ready;                    // Contiene t_pcb
extern t_sdictionary *colas_blocked_recursos;   // Contiene squeues de t_pcb
extern t_sdictionary *colas_blocked_interfases; // Contiene squeues de t_pcb

// Recursos
extern t_sdictionary *instancias_recursos; // Contiene ints.
extern t_slist *asignaciones_recursos;     // Necesito saber que procesos tienen que recursos para liberarlos
                                           // al eliminar al proceso. Contiene t_asignacion_recursos.
// Semaforos
extern sem_t sem_multiprogramacion; // Semaforo de espacio restante de multiprogamacion
extern sem_t sem_elementos_en_new;
extern sem_t sem_elementos_en_ready;
extern sem_t sem_interrupciones_activadas;

// Control sobre planificadores
// Ponemos el bool en true para hacer wait al semaforo en el loop de cada hilo.
extern bool planificacion_pausada;
extern sem_t sem_reanudar_pcp;
extern sem_t sem_reanudar_plp;

/*
** Definiciones de funciones
*/

// Inicializa y libera las variables globales.
void inicializar_globales(void);
void liberar_globales(void);

void *esperar_conexiones_io(void *);

void correr_consola(void);

// Maneja las conexiones de los dispositivos de I/O
void *atender_io(void *param);

t_algoritmo_planificacion parse_algoritmo_planifiacion(char *str);

void *planificador_largo_plazo(void *param);
void *planificador_corto_plazo(void *params);

// Solicita a memoria eliminar un proceso y libera sus recursos, incluye los logs.
void eliminar_proceso(t_pcb *pcb, int conexion_memoria);
void liberar_recursos_proceso(char *key, void *value); // Usado en un iterador.

// Incrementa las instancias y desbloquea procesos.
void liberar_recurso(char *recurso);
// Agrega el pid a la lista de asignaciones de ese recurso
void asignar_recurso(uint32_t pid, char *recurso);
// Elimina el pid de la lista de asignaciones de ese recurso
void liberar_asignacion_recurso(uint32_t pid, char *recurso);

void pausar_planificacion();
void reanudar_planificacion();

#endif // MAIN_H_
