#ifndef MAIN_H_
#define MAIN_H_

#include <assert.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <commons/memory.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/mensajes.h>
#include <utils/pcb.h>
#include <utils/slist.h>
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
        int num;
        char str[255];
    } parametros[5]; // Las instrucciones pueden tener hasta 5 parametros
} t_instruccion;

typedef struct {
    uint32_t pid;
    t_motivo_desalojo motivo;
} t_interrupcion;

typedef enum { FIFO, LRU } t_algoritmo_tlb;

/*
** Variables globales
*/
extern t_log *debug_logger;
extern t_log *cpu_logger;
extern t_config *config;

extern t_pcb *pcb;
extern t_slist *interrupts; // Contiene t_interrupcion

// Recibido de memoria al iniciar.
extern uint32_t tam_pagina;

// Contador de instrucciones, usado para LRU
extern uint64_t num_instruccion_actual;

extern int conexion_memoria;

// Variables de config
extern char *ip_memoria;
extern char *puerto_memoria;
extern char *puerto_escucha_dispatch;
extern char *puerto_escucha_interrupt;
extern int cantidad_entradas_tlb;
extern t_algoritmo_tlb algoritmo_tlb;

/*
** Funciones compartidas
*/
void inicializar_mmu(void);

// Ciclo de instruccion
char *fetch(uint32_t pid, uint32_t program_counter);
t_instruccion decode(char *instruccion);
void execute(t_instruccion instruccion_a_ejecutar, bool *incrementar_pc, int conexion_dispatch);
void check_interrupt(int conexion_dispatch);

void devolver_pcb(t_motivo_desalojo motivo, int conexion_dispatch);

/*
** MMU
*/

// Devuelve la direccion fisica, consultando la tabla de páginas en memoria si es necesario
size_t obtener_direccion_fisica(uint32_t pid, size_t dir_logica);
// Devuelve si es posible guardar tamanio pagina a partir de dir_logica.
bool entra_en_pagina(size_t dir_logica, size_t tamanio);
// Devuelve el tamanio que queda en la pagina a partir de la direccion lógica
size_t tam_restante_pag(size_t dir_logica);
// Lee tamanio bytes y devuelve el buffer.
void *leer_espacio_usuario(uint32_t pid, size_t dir_logica, size_t tamanio);
// Escribe tamanio bytes del buffer datos.
void escribir_espacio_usuario(uint32_t pid, size_t dir_logica, const void *datos, size_t tamanio);



#endif // MAIN_H_
