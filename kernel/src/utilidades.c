#include "utilidades.h"

static void *pcb_to_pidstr(void *pcb);
static void *proceso_nuevo_to_pidstr(void *proceso_nuevo);
static char *str_list_to_csv(t_list *strs);
static void *itoa_voidp(void *pid_int);

char *obtener_lista_pids_pcb(t_squeue *cola)
{
    // Bloquear el mutex mientras se leen los elementos.
    squeue_lock(cola);
    t_list *pid_strs = list_map(cola->queue->elements, pcb_to_pidstr);
    squeue_unlock(cola);

    char *str = str_list_to_csv(pid_strs);

    list_destroy_and_destroy_elements(pid_strs, free);

    return str;
}

char *obtener_lista_pids_proceso_nuevo(t_squeue *cola)
{
    // Bloquear el mutex mientras se leen los elementos.
    squeue_lock(cola);
    t_list *pid_strs = list_map(cola->queue->elements, proceso_nuevo_to_pidstr);
    squeue_unlock(cola);

    char *str = str_list_to_csv(pid_strs);

    list_destroy_and_destroy_elements(pid_strs, free);

    return str;
}

char *obtener_lista_pids_exit(t_squeue *cola)
{
    // Bloquear el mutex mientras se leen los elementos.
    squeue_lock(cola);
    t_list *pid_strs = list_map(cola->queue->elements, itoa_voidp);
    squeue_unlock(cola);

    char *str = str_list_to_csv(pid_strs);

    list_destroy_and_destroy_elements(pid_strs, free);

    return str;
}

static void *pcb_to_pidstr(void *pcb)
{
    return (void *)string_itoa(((t_pcb *)pcb)->pid);
}

static void *proceso_nuevo_to_pidstr(void *proceso_nuevo)
{
    t_proceso_nuevo *pn = ((t_proceso_nuevo *)proceso_nuevo);
    return (void *)string_itoa(pn->pid);
}

static void *itoa_voidp(void *pid_int)
{
    return string_itoa(*(uint32_t *)pid_int);
}

static char *str_list_to_csv(t_list *strs)
{
    char *str = string_new();

    t_list_iterator *it = list_iterator_create(strs);
    if (!list_iterator_has_next(it)) { // La lista tiene 0 elementos
        list_iterator_destroy(it);
        return str; // Devolvemos el string vacio
    }
    string_append(&str, list_iterator_next(it)); // Agregar el primero

    while (list_iterator_has_next(it)) {
        string_append_with_format(&str, ", %s", (char *)list_iterator_next(it));
    }

    list_iterator_destroy(it);

    return str;
}
