#ifndef DECODER_H_
#define DECODER_H_

#include <commons/string.h>
#include "main.h"
#include <string.h>

/*
** Definiciones de funciones
*/
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