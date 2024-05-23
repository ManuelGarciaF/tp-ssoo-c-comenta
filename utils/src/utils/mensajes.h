#ifndef UTILS_MENSAJES_H_
#define UTILS_MENSAJES_H_

/*
** Variables globales
*/

typedef enum { // Enviados a memoria para dar a conocer quien se conecta
    MENSAJE_A_MEMORIA_CPU,
    MENSAJE_A_MEMORIA_KERNEL,
    MENSAJE_A_MEMORIA_IO
} t_mensaje_identificacion_memoria;

typedef enum { // Enviados a memoria por kernel
    MENSAJE_INICIO_PROCESO,
    MENSAJE_FIN_PROCESO
} t_op_memoria_kernel;

typedef enum { // Enviados a memoria por cpu
    MENSAJE_SOLICITAR_INSTRUCCION
} t_op_memoria_cpu;

// Enviado a kernel por la interfaz para avisar que termino
#define MENSAJE_FIN_IO 1

typedef enum { // Enviados a kernel por cpu al devolver un pcb
    FIN_QUANTUM,
    FIN_PROCESO,
    WAIT_RECURSO,
    SIGNAL_RECURSO,
    IO,
    INTERRUMPIDO_POR_USUARIO
} t_motivo_desalojo;

typedef enum { GENERICA, STDIN, STDOUT, DIALFS } t_tipo_interfaz;

typedef enum {
    GEN_SLEEP = 0,
    STDIN_READ,
    STDOUT_WRITE,
    FS_CREATE,
    FS_DELETE,
    FS_TRUNCATE,
    FS_WRITE,
    FS_READ
} t_operacion_io;

// Le pasamos un t_operacion_io y devuelve el tipo de interfaz que puede hacer esa operacion
extern const t_tipo_interfaz TIPO_INTERFAZ_CAPAZ_DE_HACER[];

#endif
