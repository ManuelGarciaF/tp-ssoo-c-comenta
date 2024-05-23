#ifndef UTILIDADES_KERNEL_H_
#define UTILIDADES_KERNEL_H_

#include <commons/collections/list.h>
#include <utils/squeue.h>
#include <utils/pcb.h>
#include <commons/string.h>
#include "estructuras.h"
#include "utils/sdictionary.h"

// Devuelve un string con los pids separados por comas a partir de una squeue de t_pcb
char *obtener_lista_pids_pcb(t_squeue *cola);

// Devuelve un string con los pids separados por comas a partir de una squeue de t_proceso_nuevo
char *obtener_lista_pids_proceso_nuevo(t_squeue *cola);

// Imprime el nombre de cada recurso y los procesos que estan bloqueados esperandolo
void imprimir_bloqueados_por_recursos(t_sdictionary *colas_blocked_recursos);

// Imprime el nombre de cada interfaz y los procesos que estan bloqueados esperandolo
void imprimir_bloqueados_por_io(t_sdictionary *interfaces_conectadas);

#endif // UTILIDADES_KERNEL_H_
