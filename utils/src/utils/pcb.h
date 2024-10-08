#ifndef PCB_H_
#define PCB_H_

#include <stdint.h>
#include <stdlib.h>
#include <utils/sockets.h>
#include <commons/log.h>
#include <string.h>
#include <assert.h>

// Debe estar definido
extern t_log *debug_logger;

typedef struct {
    uint8_t ax;
    uint8_t bx;
    uint8_t cx;
    uint8_t dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;
    uint32_t di;
} t_registros;

typedef struct {
    uint32_t pid;
    uint32_t program_counter;
    int64_t quantum; // Milisegundos restantes para VRR, puede ser negativo
    t_registros registros;
} t_pcb;

/*
** Definiciones de funciones
*/

t_pcb *pcb_create(uint32_t pid);
void pcb_destroy(t_pcb *);

void pcb_send(t_pcb *, int socket_conexion);
t_pcb *pcb_receive(int socket_conexion);

void pcb_debug_print(t_pcb *);

#endif // PCB_H_
