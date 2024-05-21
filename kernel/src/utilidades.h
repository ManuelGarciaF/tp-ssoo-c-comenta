#ifndef UTILIDADESKERNEL_H_
#define UTILIDADESKERNEL_H_

#include <commons/collections/list.h>
#include <utils/squeue.h>
#include <estructuras/pcb.h>
#include <commons/string.h>

char *obtener_lista_pids(t_squeue *cola_ready);

void *pcb_to_pidstr(void *pcb);

#endif // UTILIDADESKERNEL_H_
