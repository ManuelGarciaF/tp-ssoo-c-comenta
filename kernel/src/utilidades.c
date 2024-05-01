#include "utilidades.h"

// TODO testear
char *obtener_lista_pids(t_squeue *cola_ready) {
    return (char *)list_fold(cola_ready->queue->elements, "", (void *)pcb_to_pidstr);
}

char *pcb_to_pidstr(char *seed, t_pcb *pcb) {
    char *pidstr = string_itoa(pcb->pid);
    // FIXME Posible leak de memoria
    string_append(&seed, pidstr);
    free(pidstr);
    string_append(&seed, ", ");
    return seed;
}
