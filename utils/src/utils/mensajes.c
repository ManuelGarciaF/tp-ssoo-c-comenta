#include "mensajes.h"

const t_tipo_interfaz TIPO_INTERFAZ_CAPAZ_DE_HACER[] = {
    GENERICA, // GEN_SLEEP
    STDIN,    // STDIN_READ
    STDOUT,   // STDOUT_WRITE
    DIALFS,   // FS_CREATE
    DIALFS,   // FS_DELETE
    DIALFS,   // FS_TRUNCATE
    DIALFS,   // FS_WRITE
    DIALFS    // FS_READ
};
