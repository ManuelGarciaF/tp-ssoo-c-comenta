#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_

#include "main.h"

// Variables globales.
uint32_t pid_en_ejecucion;
bool proceso_desalojo_previamente;

sem_t sem_comenzar_reloj;

// Definiciones de funciones.
void reloj_rr(void);

void enviar_pcb_a_cpu(t_pcb *pcb_a_ejecutar, int conexion_cpu_dispatch) ;

void desalojar_proceso();

t_motivo_desalojo recibir_pcb(int conexion_cpu_dispatch, t_pcb *pcb_actualizado, char *nombre_recurso_interfaz);

#endif // PLANIFICADOR_CORTO_PLAZO_H_
