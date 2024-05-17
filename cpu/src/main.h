#ifndef MAIN_H_
#define MAIN_H_

#include <commons/log.h>
#include <estructuras/pcb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/mensajes.h>
#include <utils/sockets.h>
#include <utils/utils.h>

/*
** Estructuras
*/
typedef enum {
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} t_opcode;

typedef enum { PC, AX, BX, CX, DX, EAX, EBX, ECX, EDX, SI, DI } t_registro;

typedef struct {
    t_opcode opcode;
    union {
        t_registro registro;
        int valor_numerico;
        char str[255];
    } parametros[5];
} t_instruccion;

/*
** Variables globales
*/
extern t_log *debug_logger;
extern t_log *cpu_logger;
extern t_pcb *pcb;

// Variables de config
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *puerto_escucha_dispatch;
extern char *puerto_escucha_interrupt;

/*
** Definiciones de funciones
*/
void cargar_config(t_config *config);

void *servidor_dispatch(int *socket_escucha);
void *servidor_interrupt(int *socket_escucha);
int aceptar_conexion_kernel(int socket_escucha);
char *fetch(uint32_t pid, uint32_t program_counter, int conexion_memoria);
t_instruccion decode(char *instruccion);
void excecute(t_instruccion instruccion_a_ejecutar);

#endif // MAIN_H_
