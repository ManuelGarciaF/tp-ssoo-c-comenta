#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_

#include "main.h"

// Variables globales.
bool proceso_desalojo_previamente = false;

sem_t sem_comenzar_reloj;

typedef struct {
    int conexion_cpu_interrupt;
    int ms_espera;
    uint32_t pid;
} t_parametros_reloj_rr;

// Definiciones de funciones.
void *reloj_rr(void *param);

void desalojar_proceso(int conexion_interrupt);

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb **pcb_actualizado);

// Motivos de devolucion de pcb
void manejar_fin_quantum(t_pcb *pcb_recibido);
void manejar_fin_proceso(t_pcb *pcb_recibido, int conexion_memoria);
void manejar_wait_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria);
void manejar_signal_recurso(t_pcb *pcb_recibido, int socket_conexion_dispatch, int conexion_memoria);
void manejar_io(t_pcb *pcb_recibido, int conexion_dispatch, int conexion_memoria);

void recurso_invalido(t_pcb *pcb_recibido, int conexion_memoria);
void interfaz_invalida(t_pcb *pcb_recibido, int conexion_memoria);

#endif // PLANIFICADOR_CORTO_PLAZO_H_
