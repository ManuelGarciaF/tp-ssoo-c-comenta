#ifndef MAIN_H_
#define MAIN_H_

#include <assert.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <commons/memory.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/bloque.h>
#include <utils/mensajes.h>
#include <utils/pcb.h>
#include <utils/slist.h>
#include <utils/sockets.h>
#include <utils/utils.h>

// Si imprimir los logs de debug por pantalla
#define PRINT_DEBUG false

//
// Estructuras
//
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
    t_opcode opcode; // El opcode define el numero de parametros y sus tipos.
    union {
        t_registro registro;
        int num;
        char str[255];
    } parametros[5]; // Las instrucciones pueden tener hasta 5 parametros.
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
extern char *IP_MEMORIA;
extern char *PUERTO_MEMORIA;
extern char *PUERTO_ESCUCHA_DISPATCH;
extern char *PUERTO_ESCUCHA_INTERRUPT;
extern int CANTIDAD_ENTRADAS_TLB;
extern t_algoritmo_tlb ALGORITMO_TLB;

//
// Funciones compartidas
//

// Ciclo de instruccion
char *fetch(uint32_t pid, uint32_t program_counter);
t_instruccion decode(char *instruccion);
void execute(t_instruccion instruccion_a_ejecutar, bool *incrementar_pc, int conexion_dispatch);
void check_interrupt(int conexion_dispatch);

void devolver_pcb(t_motivo_desalojo motivo, int conexion_dispatch);

//
// MMU
//

void inicializar_mmu(void);

// Devuelve la direccion fisica, consultando la tabla de páginas en memoria si es necesario
size_t obtener_direccion_fisica(uint32_t pid, size_t dir_logica);

// Lee tamanio bytes y devuelve el buffer, la lectura puede ocurrir a traves de mas de una pagina.
void *leer_espacio_usuario(uint32_t pid, size_t dir_logica, size_t tamanio);

// Escribe tamanio bytes del buffer datos, la escritura puede ocurrir a traves de mas de una pagina.
void escribir_espacio_usuario(uint32_t pid, size_t dir_logica, const void *datos, size_t tamanio);

// Devuelve una lista de bloques para realizar operaciones en más de una pagina en base a una direccion logica y un
// tamanio.
t_list *obtener_bloques(uint32_t pid, size_t dir_logica, size_t tamanio);

// Saca las entradas invalidadas por cambiar el tamanio de trabajo.
void remover_entradas_tlb_invalidas(uint32_t pid, size_t tamanio);

#endif // MAIN_H_
