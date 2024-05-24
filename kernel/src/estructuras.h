#ifndef ESTRUCTURAS_KERNEL_H_
#define ESTRUCTURAS_KERNEL_H_

#include "utils/mensajes.h"
#include "utils/pcb.h"
#include "utils/squeue.h"
#include <semaphore.h>
#include <stdint.h>

typedef struct {
    uint32_t pid;
    char *path;
} t_proceso_nuevo;

typedef enum { FIFO, RR, VRR } t_algoritmo_planificacion;

typedef struct {
    char *nombre_recurso;
    uint32_t pid;
} t_asignacion_recurso;

typedef struct {
    char *nombre;
    t_tipo_interfaz tipo;
    int conexion;
    sem_t procesos_esperando;
    t_squeue *bloqueados; // Contiene t_bloqueado_io, el primer elemento es el que esta usando la interfaz
} t_interfaz;

typedef struct {
    t_pcb *pcb;
    t_operacion_io opcode;
    t_list *operacion; // Contiene el nombre de la interfaz [0], la operacion [1],
                       // y el resto de los argumentos.
} t_bloqueado_io;

#endif // ESTRUCTURAS_KERNEL_H_
