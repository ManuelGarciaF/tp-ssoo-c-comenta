#ifndef EXCECUTER_H_
#define EXCECUTER_H_

#include "main.h"
#include <commons/string.h>
#include <string.h>

/*
** Definiciones de funciones
*/
uint32_t get_registro(t_registro registro);
void set_registro(t_registro registro, int valor, bool *incrementar_pc);

#endif // EXCECUTER_H_
