#include "main.h"

void liberar_recurso(char *recurso)
{
    // NOTE No necesito mutexear esto ya que los recursos solo son modificados por el pcp.
    int *cant_recurso = sdictionary_get(instancias_recursos, recurso);

    // Si es <0, hay procesos bloqueados.
    if (*cant_recurso < 0) {
        // Sacar el primer proceso de la cola de bloqueados.
        t_squeue *cola = sdictionary_get(colas_blocked_recursos, recurso);
        t_pcb *pcb = squeue_pop(cola);

        // Asignarle el recurso y agregarlo a ready.
        asignar_recurso(pcb->pid, recurso);

        squeue_push(cola_ready, pcb);
    }

    // Incrementar el contador de instancias del recurso.
    (*cant_recurso)++;
}

void asignar_recurso(uint32_t pid, char *recurso)
{
    t_asignacion_recurso *ar = malloc(sizeof(t_asignacion_recurso));
    assert(ar != NULL);
    ar->nombre_recurso = string_duplicate(recurso);
    ar->pid = pid;
    slist_add(asignaciones_recursos, ar);
}

// Libera una sola asignacion.
void liberar_asignacion_recurso(uint32_t pid, char *recurso)
{
    slist_lock(asignaciones_recursos);
    // Iterar hasta encontrar el pid y eliminarlo.
    // NOTE si no hay asignaciones no se hace nada, esto es correcto.
    t_list_iterator *it = list_iterator_create(asignaciones_recursos->list);
    while (list_iterator_has_next(it)) {
        t_asignacion_recurso *ar = list_iterator_next(it);

        // Buscar una asignacion al recurso correcto del proceso correcto
        if (ar->pid == pid && strcmp(ar->nombre_recurso, recurso) == 0) {
            list_iterator_remove(it);
            free(ar->nombre_recurso);
            free(ar);
            break;
        }
    }
    list_iterator_destroy(it);
    slist_unlock(asignaciones_recursos);
}
