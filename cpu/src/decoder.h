#ifndef DECODER_H_
#define DECODER_H_

#include <commons/string.h>
#include "main.h"
#include <string.h>

/*
** Estructuras
*/

typedef enum { SET, MOV_IN, MOV_OUT, SUM, SUB, JNZ, RESIZE, COPY_STRING, WAIT, SIGNAL, IO_GEN_SLEEP, IO_STDIN_READ, IO_STDOUT_WRITE, IO_FS_CREATE, IO_FS_DELETE, IO_FS_TRUNCATE, IO_FS_WRITE, IO_FS_READ, EXIT} t_opcode;

typedef enum { AX, BX, CX, DX, EAX, EBX, ECX, EDX, SI, DI } t_registro;

typedef struct {
    t_opcode opcode;
    union {
        t_registro registro;
        int valor_numerico;
        char str[255];
    } parametros[5];
} t_instruccion;

/*
** Definiciones de funciones
*/
t_instruccion decode(char *instruccion);
t_instruccion parsear_set(char *argumentos);
t_instruccion parsear_mov_in(char *argumentos);
t_instruccion parsear_mov_out(char *argumentos);
t_instruccion parsear_sum(char *argumentos);
t_instruccion parsear_sub(char *argumentos);
t_instruccion parsear_jnz(char *argumentos);
t_instruccion parsear_resize(char *argumentos);
t_instruccion parsear_copy_string(char *argumentos);
t_instruccion parsear_wait(char *argumentos);
t_instruccion parsear_signal(char *argumentos);
t_instruccion parsear_io_gen_sleep(char *argumentos);
t_instruccion parsear_io_stdin_read(char *argumentos);
t_instruccion parsear_io_stdout_write(char *argumentos);
t_instruccion parsear_io_fs_create(char *argumentos);
t_instruccion parsear_io_fs_delete(char *argumentos);
t_instruccion parsear_io_fs_truncate(char *argumentos);
t_instruccion parsear_io_fs_write(char *argumentos);
t_instruccion parsear_io_fs_read(char *argumentos);
t_instruccion parsear_exit(char *argumentos);

t_registro parsear_a_t_registro(char *str);

#endif // DECODER_H_