#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <utils/slist.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    t_list *codigo; // Contiene las lineas del pseudocodigo
    t_slist *paginas; // Contiene ints, el indice es el numero de pagina, el valor es el numero de marco
} t_proceso;

t_proceso *proceso_create(t_list *lineas_codigo);
void proceso_destroy(t_proceso *data);

#endif // ESTRUCTURAS_H_
