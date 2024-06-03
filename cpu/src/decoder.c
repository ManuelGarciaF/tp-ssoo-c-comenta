#include "decoder.h"

t_instruccion decode(char *instruccion)
{
    char **opcode_y_parametros = string_n_split(instruccion, 2, " ");
    t_instruccion ret;

    if (!strcmp(opcode_y_parametros[0], "SET")) {
        ret = parsear_set(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "MOV_IN")) {
        ret = parsear_mov_in(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "MOV_OUT")) {
        ret = parsear_mov_out(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "SUM")) {
        ret = parsear_sum(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "SUB")) {
        ret = parsear_sub(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "JNZ")) {
        ret = parsear_jnz(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "RESIZE")) {
        ret = parsear_resize(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "COPY_STRING")) {
        ret = parsear_copy_string(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "WAIT")) {
        ret = parsear_wait(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "SIGNAL")) {
        ret = parsear_signal(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_GEN_SLEEP")) {
        ret = parsear_io_gen_sleep(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_STDIN_READ")) {
        ret = parsear_io_stdin_read(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_STDOUT_WRITE")) {
        ret = parsear_io_stdout_write(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_FS_CREATE")) {
        ret = parsear_io_fs_create(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_FS_DELETE")) {
        ret = parsear_io_fs_delete(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_FS_TRUNCATE")) {
        ret = parsear_io_fs_truncate(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_FS_WRITE")) {
        ret = parsear_io_fs_write(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "IO_FS_READ")) {
        ret = parsear_io_fs_read(opcode_y_parametros[1]);
    } else if (!strcmp(opcode_y_parametros[0], "EXIT")) {
        ret = parsear_exit(opcode_y_parametros[1]);
    } else {
        log_error(debug_logger, "Programa contiene una instruccion invalida");
        abort();
    }

    // Liberar memoria
    string_array_destroy(opcode_y_parametros);

    return ret;
}

t_instruccion parsear_set(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro registro = parsear_a_t_registro(argumentos_separados[0]);
    int valor = atoi(argumentos_separados[1]);

    instruccion.opcode = SET;
    instruccion.parametros[0].registro = registro;
    instruccion.parametros[1].valor_numerico = valor;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_mov_in(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro datos = parsear_a_t_registro(argumentos_separados[0]);
    t_registro direccion = parsear_a_t_registro(argumentos_separados[1]);

    instruccion.opcode = MOV_IN;
    instruccion.parametros[0].registro = datos;
    instruccion.parametros[1].registro = direccion;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_mov_out(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro direccion = parsear_a_t_registro(argumentos_separados[0]);
    t_registro datos = parsear_a_t_registro(argumentos_separados[1]);

    instruccion.opcode = MOV_OUT;
    instruccion.parametros[0].registro = direccion;
    instruccion.parametros[1].registro = datos;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_sum(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro destino = parsear_a_t_registro(argumentos_separados[0]);
    t_registro origen = parsear_a_t_registro(argumentos_separados[1]);

    instruccion.opcode = SUM;
    instruccion.parametros[0].registro = destino;
    instruccion.parametros[1].registro = origen;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_sub(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro destino = parsear_a_t_registro(argumentos_separados[0]);
    t_registro origen = parsear_a_t_registro(argumentos_separados[1]);

    instruccion.opcode = SUB;
    instruccion.parametros[0].registro = destino;
    instruccion.parametros[1].registro = origen;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_jnz(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro registro = parsear_a_t_registro(argumentos_separados[0]);
    int numero_instruccion = atoi(argumentos_separados[1]);

    instruccion.opcode = JNZ;
    instruccion.parametros[0].registro = registro;
    instruccion.parametros[1].valor_numerico = numero_instruccion;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_resize(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    int tamanio = atoi(argumentos_separados[0]);

    instruccion.opcode = RESIZE;
    instruccion.parametros[0].valor_numerico = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_copy_string(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    int tamanio = atoi(argumentos_separados[0]);

    instruccion.opcode = COPY_STRING;
    instruccion.parametros[0].valor_numerico = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_wait(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = WAIT;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_signal(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = SIGNAL;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_gen_sleep(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    int unidades_de_trabajo = atoi(argumentos_separados[1]);

    instruccion.opcode = IO_GEN_SLEEP;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    instruccion.parametros[1].valor_numerico = unidades_de_trabajo;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_stdin_read(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro direccion = parsear_a_t_registro(argumentos_separados[1]);
    t_registro tamanio = parsear_a_t_registro(argumentos_separados[2]);

    instruccion.opcode = IO_STDIN_READ;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    instruccion.parametros[1].registro = direccion;
    instruccion.parametros[2].registro = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_stdout_write(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro direccion = parsear_a_t_registro(argumentos_separados[1]);
    t_registro tamanio = parsear_a_t_registro(argumentos_separados[2]);

    instruccion.opcode = IO_STDOUT_WRITE;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    instruccion.parametros[1].registro = direccion;
    instruccion.parametros[2].registro = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_fs_create(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = IO_FS_CREATE;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    strcpy(instruccion.parametros[1].str, argumentos_separados[1]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_fs_delete(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = IO_FS_DELETE;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    strcpy(instruccion.parametros[1].str, argumentos_separados[1]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_fs_truncate(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    t_registro tamanio = parsear_a_t_registro(argumentos_separados[2]);

    instruccion.opcode = IO_FS_TRUNCATE;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    strcpy(instruccion.parametros[1].str, argumentos_separados[1]);
    instruccion.parametros[2].registro = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_fs_write(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = IO_FS_WRITE;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    strcpy(instruccion.parametros[1].str, argumentos_separados[1]);
    instruccion.parametros[2].registro = parsear_a_t_registro(argumentos_separados[2]);
    instruccion.parametros[3].registro = parsear_a_t_registro(argumentos_separados[3]);
    instruccion.parametros[4].registro = parsear_a_t_registro(argumentos_separados[4]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_fs_read(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_split(argumentos, " ");

    instruccion.opcode = IO_FS_READ;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    strcpy(instruccion.parametros[1].str, argumentos_separados[1]);
    instruccion.parametros[2].registro = parsear_a_t_registro(argumentos_separados[2]);
    instruccion.parametros[3].registro = parsear_a_t_registro(argumentos_separados[3]);
    instruccion.parametros[4].registro = parsear_a_t_registro(argumentos_separados[4]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_exit(char *argumentos)
{
    t_instruccion instruccion = {0};

    instruccion.opcode = EXIT;

    return instruccion;
}

t_registro parsear_a_t_registro(char *str)
{
    if (strcmp(str, "AX") == 0) {
        return AX;
    }
    if (strcmp(str, "BX") == 0) {
        return BX;
    }
    if (strcmp(str, "CX") == 0) {
        return CX;
    }
    if (strcmp(str, "DX") == 0) {
        return DX;
    }
    if (strcmp(str, "EAX") == 0) {
        return EAX;
    }
    if (strcmp(str, "EBX") == 0) {
        return EBX;
    }
    if (strcmp(str, "ECX") == 0) {
        return ECX;
    }
    if (strcmp(str, "EDX") == 0) {
        return EDX;
    }
    if (strcmp(str, "SI") == 0) {
        return SI;
    }
    if (strcmp(str, "DI") == 0) {
        return DI;
    }

    abort(); // Unreachble
}
