#ifndef CONSOLA_H_
#define CONSOLA_H_

#include "main.h"
#include <commons/string.h>

// Funciones
void ejecutar_comando(char const *linea);
void iniciar_proceso(char const *path);
void finalizar_proceso(char const *pid);

void actualizar_grado_multiprogramacion(int nuevo);

#endif // CONSOLA_H_
