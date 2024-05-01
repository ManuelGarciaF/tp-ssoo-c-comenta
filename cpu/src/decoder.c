#include "decoder.h"

void decode(char *instruccion) {
    char **instruccion_parametrizada = string_n_split(instruccion, 2, " ");

    if (!strcmp(instruccion_parametrizada[0], "SET")){
       // SI
    } else if (!strcmp(instruccion_parametrizada[0], "MOV_IN")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "MOV_OUT")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "SUM")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "SUB")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "JNZ")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "RESIZE")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "COPY_STRING")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "WAIT")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "SIGNAL")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "IO_GEN_SLEEP")) {
        //SI
    } else if (!strcmp(instruccion_parametrizada[0], "IO_STDIN_READ")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "IO_STDOUT_WRITE")) {
        //ENTREGA 3
    } else if (!strcmp(instruccion_parametrizada[0], "IO_FS_CREATE")) {
        //ENTREGA FINAL
    } else if (!strcmp(instruccion_parametrizada[0], "IO_FS_DELETE")) {
            //ENTREGA FINAL
    } else if (!strcmp(instruccion_parametrizada[0], "IO_FS_TRUNCATE")) {
        //ENTREGA FINAL
    } else if (!strcmp(instruccion_parametrizada[0], "IO_FS_WRITE")) {
        //ENTREGA FINAL
    } else if (!strcmp(instruccion_parametrizada[0], "IO_FS_READ")) {
        //ENTREGA FINAL
    } else if (!strcmp(instruccion_parametrizada[0], "EXIT")) {
        //SI
    } 


}