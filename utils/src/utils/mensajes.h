#ifndef UTILS_MENSAJES_H_
#define UTILS_MENSAJES_H_

/*
** Variables globales
*/

#define MENSAJE_A_MEMORIA_CPU "cpu"
#define MENSAJE_A_MEMORIA_KERNEL "kernel"
#define MENSAJE_A_MEMORIA_IO "io"

#define MENSAJE_INICIO_PROCESO "inicio"
#define MENSAJE_FIN_PROCESO "fin"

typedef enum { FIN_QUANTUM, FIN_PROCESO, WAIT_RECURSO, SIGNAL_RECURSO, IO } t_motivo_desalojo;

#endif
