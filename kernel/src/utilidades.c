#include "utilidades.h"

char *obtener_lista_pids(t_squeue *cola_ready) {
    t_list *pid_strs = list_map(cola_ready->queue->elements, (void *)pcb_to_pidstr);

    char *str = string_new();

    t_list_iterator *iterator = list_iterator_create(pid_strs);
    while (list_iterator_has_next(iterator)) {
        // No agregar la coma antes del primero
        if (list_iterator_index(iterator) == 0) {
            string_append(&str, ", ");
        }

        // Agregar el pid
        string_append(&str, list_iterator_next(iterator));
    }

    list_iterator_destroy(iterator);
    list_destroy_and_destroy_elements(pid_strs, free);

    return str;
}

char *pcb_to_pidstr(t_pcb *pcb) {
    return string_itoa(pcb->pid);
}
