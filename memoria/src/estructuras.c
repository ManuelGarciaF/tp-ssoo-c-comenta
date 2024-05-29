#include "estructuras.h"

t_proceso *proceso_create(t_list *lineas_codigo)
{
    t_proceso *proceso = malloc(sizeof(t_proceso));
    assert(proceso != NULL);
    proceso->codigo = lineas_codigo;
    proceso->paginas = list_create();

    return proceso;
}

void proceso_destroy(t_proceso *proceso)
{
    list_destroy_and_destroy_elements(proceso->codigo, free);
    list_destroy_and_destroy_elements(proceso->paginas, free);
    free(proceso);
}
