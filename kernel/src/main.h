#ifndef MAIN_H_
#define MAIN_H_

#include "estructuras.h"
#include "utilidades.h"

#include <assert.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/mensajes.h>
#include <utils/pcb.h>
#include <utils/sdictionary.h>
#include <utils/slist.h>
#include <utils/sockets.h>
#include <utils/squeue.h>
#include <utils/utils.h>

/*
** Variables globales
*/
extern t_log *debug_logger;
extern t_log *kernel_logger;
extern t_config *config;

// Variables de config
extern char *PUERTO_ESCUCHA;
extern char *IP_MEMORIA;
extern char *PUERTO_MEMORIA;
extern char *IP_CPU;
extern char *PUERTO_CPU_DISPATCH;
extern char *PUERTO_CPU_INTERRUPT;
extern t_algoritmo_planificacion ALGORITMO_PLANIFICACION;
extern int QUANTUM;
extern int grado_multiprogramacion; // Minuscula porque se modifica :)

extern int pid_en_ejecucion;
extern int procesos_extra_multiprogramacion; // Si se achico el grado_multiprogramacion, los proximos procesos que
                                             // se eliminen no liberan espacio en el sem_multiprogramacion.

extern int conexion_memoria;
extern pthread_mutex_t mutex_conexion_memoria; // Necesitamos asegurar que solo un hilo puede comunicarse con la
                                               // memoria a la vez
extern int conexion_cpu_dispatch;              // No hace falta mutex, solo es usado por el pcp
extern int conexion_cpu_interrupt;             // Usamos sem_interrupcion_rr como mutex

// Colas de procesos
extern t_squeue *cola_new;                    // Contiene t_proceso_nuevo
extern t_squeue *cola_ready;                  // Contiene t_pcb
extern t_squeue *cola_ready_plus;             // Contiene t_pcb
extern t_sdictionary *colas_blocked_recursos; // Contiene squeues de t_pcb
extern t_sdictionary *interfaces_conectadas;  // Contiene t_interfaz, cada interfaz tiene una squeue de bloqueados
extern t_slist *nombres_interfaces;           // Contiene strings de los nombres,
                                              // usado para poder iterar sobre los diccionarios.
extern t_squeue *cola_exit;

// Recursos
extern t_sdictionary *instancias_recursos; // Contiene ints.
extern t_slist *asignaciones_recursos;     // Necesito saber que procesos tienen que recursos para liberarlos
                                           // al eliminar al proceso. Contiene t_asignacion_recursos.
extern t_slist *nombres_recursos;          // Contiene strings de los nombres,
                                           // usado para poder iterar sobre los diccionarios

// Semaforos
extern sem_t sem_multiprogramacion; // Semaforo de espacio restante de multiprogamacion
extern sem_t sem_elementos_en_new;
extern sem_t sem_elementos_en_ready;

// Control sobre movimientos entre estados (para pausar la planificacion)
extern sem_t sem_entrada_a_ready;
extern sem_t sem_entrada_a_exec;
extern sem_t sem_manejo_desalojo_cpu;
extern sem_t sem_interrupcion_rr;
extern bool planificacion_pausada; // Para registrar el estado de la planificacion.

/*
** Funciones compartidas
*/
void correr_consola(void);

// Maneja la conexion de cada dispositivo de I/O
void *atender_io(void *param);

void *planificador_largo_plazo(void *param);
void *planificador_corto_plazo(void *params);

// Solicita a memoria eliminar un proceso y libera sus recursos, también libera el pcb.
void eliminar_proceso(t_pcb *pcb);

// Incrementa las instancias y desbloquea procesos.
void liberar_recurso(uint32_t pid, char *recurso);
// Baja las instancias y bloquea el proceso si no hay recursos disponibles.
// Devuelve si el proceso fue bloqueado o no
bool asignar_recurso(t_pcb *pcb, char *recurso);
// Libera todas las asignaciones que tiene un proceso
void liberar_asignaciones_recurso(uint32_t pid);

void pausar_planificacion(void);
void reanudar_planificacion(void);

// Interfaces
bool existe_interfaz(char *nombre);
bool interfaz_soporta_operacion(char *nombre, t_operacion_io op);

void log_cola_ready(void);
void log_cola_ready_plus(void);

void agregar_a_exit(uint32_t pid_val);

#endif // MAIN_H_
