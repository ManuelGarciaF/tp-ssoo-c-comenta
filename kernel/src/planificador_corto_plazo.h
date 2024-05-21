#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_

#include "main.h"

// Variables globales.
bool proceso_desalojo_previamente = false;

sem_t sem_comenzar_reloj;

// Definiciones de funciones.
void reloj_rr(int conexion_cpu_interrupt);

void desalojar_proceso(int conexion_interrupt);

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb **pcb_actualizado);

// Motivos de devolucion de pcb
void manejar_fin_quantum(t_pcb *pcb_recibido);
void manejar_fin_proceso(t_pcb *pcb_recibido, int conexion_memoria);
void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria);
void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria);
void manejar_io(t_pcb *pcb_recibido);

void recurso_invalido(t_pcb *pcb_recibido, int conexion_memoria);

// Agrega el pid a la lista de asignaciones de ese recurso
void asignar_recurso(uint32_t pid, char *recurso);
// Elimina el pid de la lista de asignaciones de ese recurso
void liberar_asignacion_recurso(uint32_t pid, char *recurso);

#endif // PLANIFICADOR_CORTO_PLAZO_H_
