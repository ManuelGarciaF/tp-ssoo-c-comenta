#ifndef DECODER_H_
#define DECODER_H_

#include <commons/string.h>
#include "main.h"
#include <string.h>

typedef enum { SET, SUM, SUB, JNZ, WAIT, SIGNAL, IO_GEN_SLEEP, EXIT } t_opcode;

typedef struct {
    t_opcode opcode;
    union {
        t_registro registro;
        int valor_numerico;
        char str[255];
    } parametros[5];
} t_instruccion;


typedef enum { AX, BX, CX, DX, EAX, EBX, ECX, EDX, SI, DI } t_registro;
/*
** Definiciones de funciones
*/
t_instruccion parsear_set(char **argumentos);
t_instruccion parsear_sum(char **argumentos);
t_instruccion parsear_sub(char **argumentos);
t_instruccion parsear_jnz(char **argumentos);
t_registro parsear_a_t_registro(char *str);

#endif // DECODER_H_