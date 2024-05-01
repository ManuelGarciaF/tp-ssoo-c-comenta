#ifndef UTILS_H_
#define UTILS_H_

#include <commons/collections/list.h>
#include <utils/squeue.h>
#include <estructuras/pcb.h>
#include <commons/string.h>

char *obtener_lista_pids(t_squeue *cola_ready);

char *pcb_to_pidstr(char *seed, t_pcb *pcb);

#endif // UTILS_H_
