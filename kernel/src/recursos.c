#include "main.h"

static void liberar_asignacion_recurso(uint32_t pid, char *recurso);
static void guardar_asignacion_recurso(uint32_t pid, char *recurso);
static void incrementar_recurso(uint32_t pid, char *recurso);

// NOTE No necesito mirar los semaforos de permiso para agregar a ready, ya que estas
// funciones solo se ejecutan como parte del pcp o en eliminar_proceso, en cuyo caso
// tampoco quiero que bloquee.

void liberar_recurso(uint32_t pid, char *recurso)
{
    liberar_asignacion_recurso(pid, recurso);
    incrementar_recurso(pid, recurso);
}

bool asignar_recurso(t_pcb *pcb, char *recurso)
{
    int *cant_recurso = sdictionary_get(instancias_recursos, recurso);

    log_debug(debug_logger, "PID: %d - Esperando recurso %s, valor: %d", pcb->pid, recurso, *cant_recurso);

    // Reducir el contador del recurso.
    (*cant_recurso)--;

    // Si quedaban instancias del recurso (era >0 antes de reducir).
    if (*cant_recurso >= 0) {
        // Asignar el recurso, no bloquea
        guardar_asignacion_recurso(pcb->pid, recurso);
        return false;
    }

    // Si el contador quedo en un numero negativo, el proceso bloquea
    return true;
}

void liberar_asignaciones_recurso(uint32_t pid)
{
    // Buscar asignaciones y guardarlas en una lista.
    t_list *recursos_asignados = list_create();
    slist_lock(asignaciones_recursos);
    t_list_iterator *it = list_iterator_create(asignaciones_recursos->list);
    while (list_iterator_has_next(it)) {
        t_asignacion_recurso *ar = list_iterator_next(it);

        if (ar->pid == pid) {
            list_iterator_remove(it);

            list_add(recursos_asignados, ar->nombre_recurso);

            free(ar);
        }
    }
    list_iterator_destroy(it);
    slist_unlock(asignaciones_recursos);

    // Liberar las asignaciones que obtuvimos antes
    it = list_iterator_create(recursos_asignados);
    while (list_iterator_has_next(it)) {
        char *nombre_recurso = list_iterator_next(it);
        incrementar_recurso(pid, nombre_recurso);
    }
    list_iterator_destroy(it);
    list_destroy_and_destroy_elements(recursos_asignados, free);
}

// Guarda una asignacion de recurso
static void guardar_asignacion_recurso(uint32_t pid, char *recurso)
{
    t_asignacion_recurso *ar = malloc(sizeof(t_asignacion_recurso));
    if (ar == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    ar->nombre_recurso = string_duplicate(recurso);
    ar->pid = pid;
    slist_add(asignaciones_recursos, ar);
}

// Libera una sola asignacion.
static void liberar_asignacion_recurso(uint32_t pid, char *recurso)
{
    slist_lock(asignaciones_recursos);
    // Iterar hasta encontrar el pid y eliminarlo.
    // NOTE si no hay asignaciones no falla
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

// Incrementa el recurso, desbloqueando procesos si es necesario
static void incrementar_recurso(uint32_t pid, char *recurso)
{
    int *cant_recurso = sdictionary_get(instancias_recursos, recurso);

    log_info(debug_logger, "PID: %u - Liberando recurso %s, cant. antes de incrementar: %d", pid, recurso, *cant_recurso);

    // Si es <0, hay procesos bloqueados.
    if (*cant_recurso < 0) {
        // Sacar el primer proceso de la cola de bloqueados.
        t_squeue *cola = sdictionary_get(colas_blocked_recursos, recurso);
        t_pcb *pcb = squeue_pop(cola);

        log_info(debug_logger, "Desbloqueando proceso %u", pcb->pid);

        // Asignarle el recurso y agregarlo a ready.
        guardar_asignacion_recurso(pcb->pid, recurso);

        log_info(kernel_logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->pid);
        log_cola_ready();

        squeue_push(cola_ready, pcb);
        sem_post(&sem_elementos_en_ready);
    }

    // Incrementar el contador de instancias del recurso.
    (*cant_recurso)++;
}
