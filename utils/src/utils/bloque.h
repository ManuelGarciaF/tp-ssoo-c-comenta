#ifndef BLOQUE_H
#define BLOQUE_H

#include <stdlib.h>

typedef struct {
    size_t base;    // Dirección Fisica donde inicia.
    size_t tamanio; // Bytes hasta el final.
} t_bloque;

#endif // BLOQUE_H
