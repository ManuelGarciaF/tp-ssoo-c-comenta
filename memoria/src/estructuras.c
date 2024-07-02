#include "estructuras.h"

t_proceso *proceso_create(t_list *lineas_codigo)
{
    t_proceso *proceso = malloc(sizeof(t_proceso));
    if (proceso == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    proceso->codigo = lineas_codigo;
    proceso->paginas = slist_create();

    return proceso;
}

void proceso_destroy(t_proceso *proceso)
{
    list_destroy_and_destroy_elements(proceso->codigo, free);
    slist_clean_and_destroy_elements(proceso->paginas, free);
    free(proceso);
}
