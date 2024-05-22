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

#define MENSAJE_SOLICITAR_INSTRUCCION "instruccion"

#define MENSAJE_FIN_IO_SLEEP "fin sleep"

typedef enum { FIN_QUANTUM, FIN_PROCESO, WAIT_RECURSO, SIGNAL_RECURSO, IO } t_motivo_desalojo;

typedef enum {
    GENERICA,
    STDIN,
    STDOUT,
    DIALFS
} t_tipo_interfaz;

typedef enum {
    GEN_SLEEP,
    STDIN_READ,
    STDOUT_WRITE,
    FS_CREATE,
    FS_DELETE,
    FS_TRUNCATE,
    FS_WRITE,
    FS_READ
} t_operacion_io;

#endif
