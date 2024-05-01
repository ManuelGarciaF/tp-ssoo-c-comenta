#ifndef DECODER_H_
#define DECODER_H_

#include <commons/string.h>
#include "main.h"
#include <string.h>

typedef enum { SET, MOV, ADD, SUB } t_opcode;

typedef struct {
    t_opcode opcode;
    char **parametros;
} instruccion;

#endif // DECODER_H_