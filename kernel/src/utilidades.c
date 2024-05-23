#include "utilidades.h"

static void *pcb_to_pidstr(void *pcb);
static void *proceso_nuevo_to_pidstr(void *proceso_nuevo);
static char *str_list_to_csv(t_list *strs);
static void imprimir_bloqueados_recurso(char *nombre_recurso, void *puntero_cola);
static void imprimir_bloqueados_interfaz(char *nombre_interfaz, void *puntero_interfaz);

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

void imprimir_bloqueados_por_recursos(t_sdictionary *colas_blocked_recursos)
{
    sdictionary_iterator(colas_blocked_recursos, imprimir_bloqueados_recurso);
}

void imprimir_bloqueados_por_io(t_sdictionary *interfaces_conectadas)
{
    sdictionary_iterator(interfaces_conectadas, imprimir_bloqueados_interfaz);
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

static void imprimir_bloqueados_recurso(char *nombre_recurso, void *puntero_cola)
{
    t_squeue *bloqueados = puntero_cola;

    printf(" │ └%s: ", nombre_recurso);

    squeue_lock(bloqueados);

    t_list_iterator *it = list_iterator_create(bloqueados->queue->elements);
    if (list_iterator_has_next(it)) { // La lista tiene al menos 1 elemento
        t_pcb *primer_pcb = list_iterator_next(it);
        printf("[%d", primer_pcb->pid);
        while (list_iterator_has_next(it)) {
            t_pcb *pcb = list_iterator_next(it);
            printf(", %d", pcb->pid);
        }
        printf("]\n");
    } else { // La lista esta vacia
        printf("[]\n");
    }
    list_iterator_destroy(it);
    squeue_unlock(bloqueados);
}

static void imprimir_bloqueados_interfaz(char *nombre_interfaz, void *puntero_interfaz)
{
    t_interfaz *interfaz = puntero_interfaz;

    printf("   └%s: ", nombre_interfaz);

    squeue_lock(interfaz->bloqueados);

    t_list_iterator *it = list_iterator_create(interfaz->bloqueados->queue->elements);
    if (list_iterator_has_next(it)) { // La lista tiene al menos 1 elemento
        t_bloqueado_io *primer_bloqueado = list_iterator_next(it);
        printf("[%d", primer_bloqueado->pcb->pid);
        while (list_iterator_has_next(it)) {
            t_pcb *pcb = list_iterator_next(it);
            printf(", %d", primer_bloqueado->pcb->pid);
        }
        printf("]\n");
    } else { // La lista esta vacia
        printf("[] \n");
    }
    list_iterator_destroy(it);
    squeue_unlock(interfaz->bloqueados);
}
