#include "decoder.h"

void decode(char *instruccion) {
    char **instruccion_parametrizada = string_n_split(instruccion, 2, " ");

    if (!strcmp(instruccion_parametrizada[0], "SET")){
       return parsear_set(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "MOV_IN")) {
        return parsear_mov_in(instruccion_parametrizada[1]);
    }
    else if (!strcmp(instruccion_parametrizada[0], "MOV_OUT")) {
        return parsear_mov_out(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "SUM")) {
        return parsear_sum(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "SUB")) {
        return parsear_sub(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "JNZ")) {
        return parsear_jnz(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "RESIZE")) {
        return parsear_resize(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "COPY_STRING")) {
        return parsear_copy_string(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "WAIT")) {
        //SI
    } 
    else if (!strcmp(instruccion_parametrizada[0], "SIGNAL")) {
        //SI
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_GEN_SLEEP")) {
        parsear_io_gen_sleep(instruccion_parametrizada[1]);
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_STDIN_READ")) {
        //ENTREGA 3
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_STDOUT_WRITE")) {
        //ENTREGA 3
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_FS_CREATE")) {
        //ENTREGA FINAL
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_FS_DELETE")) {
        //ENTREGA FINAL
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_FS_TRUNCATE")) {
        //ENTREGA FINAL
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_FS_WRITE")) {
        //ENTREGA FINAL
    } 
    else if (!strcmp(instruccion_parametrizada[0], "IO_FS_READ")) {
        //ENTREGA FINAL
    } 
    else if (!strcmp(instruccion_parametrizada[0], "EXIT")) {
        //SI
    } 

    //liberar memoria
    string_array_destroy(instruccion_parametrizada);
}

t_instruccion parsear_set(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    t_registro registro = parsear_a_t_registro(argumentos_separados[0]);
    int valor = atoi(argumentos_separados[1]);

    instruccion.opcode = SET;
    instruccion.parametros[0].registro = registro;
    instruccion.parametros[1].valor_numerico = valor;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_sum(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
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
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
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
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    t_registro registro = parsear_a_t_registro(argumentos_separados[0]);
    int numero_instruccion = atoi(argumentos_separados[1]);

    instruccion.opcode = JNZ;
    instruccion.parametros[0].registro = registro;
    instruccion.parametros[1].valor_numerico = numero_instruccion;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_io_gen_sleep(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    int unidades_de_trabajo = atoi(argumentos_separados[1]);

    instruccion.opcode = SET;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);
    instruccion.parametros[1].valor_numerico = unidades_de_trabajo;
    
    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_mov_in(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
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
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    t_registro direccion = parsear_a_t_registro(argumentos_separados[0]);
    t_registro datos = parsear_a_t_registro(argumentos_separados[1]);

    instruccion.opcode = MOV_IN;
    instruccion.parametros[0].registro = direccion;
    instruccion.parametros[1].registro = datos;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_resize(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    int tamanio = atoi(argumentos_separados[0]);
    
    instruccion.opcode = RESIZE;
    instruccion.parametros[0].valor_numerico = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_copy_string(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    int tamanio = atoi(argumentos_separados[0]);
    
    instruccion.opcode = COPY_STRING;
    instruccion.parametros[0].valor_numerico = tamanio;

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_wait(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    
    instruccion.opcode = COPY_STRING;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);

    string_array_destroy(argumentos_separados);
    return instruccion;
}

t_instruccion parsear_signal(char *argumentos)
{
    t_instruccion instruccion = {0};
    char **argumentos_separados = string_n_split(argumentos, 2, " ");
    
    instruccion.opcode = COPY_STRING;
    strcpy(instruccion.parametros[0].str, argumentos_separados[0]);

    string_array_destroy(argumentos_separados);
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

    exit(1);
}