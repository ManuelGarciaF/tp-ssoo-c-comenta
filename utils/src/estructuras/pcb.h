#ifndef PCB_H_
#define PCB_H_

#include <stdint.h>

typedef struct {
    uint32_t pc;
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
    uint32_t programCounter;
    uint32_t quantum;
    t_registros registros;
} t_pcb;

#endif // PCB_H_
