#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_

#include "main.h"

// Variables globales.
uint32_t pid_en_ejecucion;
bool proceso_desalojo_previamente = false;

sem_t sem_comenzar_reloj;

// Definiciones de funciones.
void reloj_rr(int conexion_cpu_interrupt);

void desalojar_proceso(int conexion_interrupt);

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb *pcb_actualizado, char *nombre_recurso_interfaz);

#endif // PLANIFICADOR_CORTO_PLAZO_H_
