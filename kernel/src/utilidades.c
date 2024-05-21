#include "utilidades.h"

char *obtener_lista_pids(t_squeue *cola_ready)
{
    // Bloquear el mutex mientras se leen los elementos.
    pthread_mutex_lock(cola_ready->mutex);
    t_list *pid_strs = list_map(cola_ready->queue->elements, pcb_to_pidstr);
    pthread_mutex_unlock(cola_ready->mutex);

    char *str = string_new();

    t_list_iterator *iterator = list_iterator_create(pid_strs);
    while (list_iterator_has_next(iterator)) {
        // No agregar la coma antes del primero
        if (list_iterator_index(iterator) != -1) {
            string_append(&str, ", ");
        }

        // Agregar el pid
        string_append(&str, list_iterator_next(iterator));
    }

    list_iterator_destroy(iterator);
    list_destroy_and_destroy_elements(pid_strs, free);

    return str;
}

void *pcb_to_pidstr(void *pcb)
{
    return (void *)string_itoa(((t_pcb *)pcb)->pid);
}
