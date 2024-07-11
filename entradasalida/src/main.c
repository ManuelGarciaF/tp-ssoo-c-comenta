#include "./main.h"

/*
** Variables globales
*/

t_log *debug_logger;
t_log *entradasalida_logger;

// Variables de config
t_tipo_interfaz TIPO_INTERFAZ;
char *NOMBRE_INTERFAZ;
int TIEMPO_UNIDAD_TRABAJO;
char *IP_KERNEL;
char *PUERTO_KERNEL;
char *IP_MEMORIA;
char *PUERTO_MEMORIA;
char *PATH_BASE_DIALFS;
int BLOCK_SIZE;
int BLOCK_COUNT;
int RETRASO_COMPACTACION;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Faltan operandos");
        printf("\'Los operandos deben ser pasados como (1) nombre_interfaz (2) "
               "archivo_config\'");
        exit(1);
    }

    NOMBRE_INTERFAZ = argv[1];
    char *archivo_config = argv[2];
    char *archivo_logger_debug = string_duplicate(NOMBRE_INTERFAZ);
    string_append(&archivo_logger_debug, "_debug.log");
    debug_logger = log_create(archivo_logger_debug, "debug", false, LOG_LEVEL_DEBUG);

    char *archivo_logger = string_duplicate(NOMBRE_INTERFAZ);
    string_append(&archivo_logger, ".log");
    entradasalida_logger = log_create(archivo_logger, "entradasalida", true, LOG_LEVEL_INFO);

    t_config *config = config_create(archivo_config);
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        abort();
    }

    cargar_config(config);

    // Conexion con el kernel
    int conexion_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
    if (!realizar_handshake(conexion_kernel)) {
        log_error(debug_logger, "No se pudo realizar un handshake con el Kernel");
    }

    t_paquete *paquete = crear_paquete();
    // Enviar nombre_interfaz y tipo_interfaz
    agregar_a_paquete(paquete, NOMBRE_INTERFAZ, strlen(NOMBRE_INTERFAZ) + 1);
    agregar_a_paquete(paquete, &TIPO_INTERFAZ, sizeof(t_tipo_interfaz));
    enviar_paquete(paquete, conexion_kernel);
    eliminar_paquete(paquete);

    // Conexion con la memoria
    int conexion_memoria = -1;
    if (TIPO_INTERFAZ != GENERICA) {
        conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
        if (!realizar_handshake(conexion_memoria)) {
            log_error(debug_logger, "No se pudo realizar un handshake con la memoria");
        }
        enviar_int(MENSAJE_A_MEMORIA_IO, conexion_memoria);
    }

    switch (TIPO_INTERFAZ) {
    case GENERICA:
        manejar_generica(conexion_kernel);
        break;
    case STDIN:
        manejar_stdin(conexion_kernel, conexion_memoria);
        break;
    case STDOUT:
        manejar_stdout(conexion_kernel, conexion_memoria);
        break;
    case DIALFS:
        manejar_dialfs(conexion_kernel, conexion_memoria);
        break;
    default:
        abort();
    }

    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(entradasalida_logger);
    config_destroy(config);
    return 0;
}

void cargar_config(t_config *config)
{
    // Propiedades Generales (van a estar en todos los tipos de interfaz)
    TIPO_INTERFAZ = parsear_a_t_tipo_interfaz(config_get_string_or_exit(config, "TIPO_INTERFAZ"));
    IP_KERNEL = config_get_string_or_exit(config, "IP_KERNEL");
    PUERTO_KERNEL = config_get_string_or_exit(config, "PUERTO_KERNEL");

    switch (TIPO_INTERFAZ) {
    case GENERICA:
        log_debug(debug_logger, "Interfaz generica");
        TIEMPO_UNIDAD_TRABAJO = config_get_int_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        break;
    case STDIN:
        log_debug(debug_logger, "Interfaz stdin");
        IP_MEMORIA = config_get_string_or_exit(config, "IP_MEMORIA");
        PUERTO_MEMORIA = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        break;
    case STDOUT:
        log_debug(debug_logger, "Interfaz stdout");
        TIEMPO_UNIDAD_TRABAJO = config_get_int_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        IP_MEMORIA = config_get_string_or_exit(config, "IP_MEMORIA");
        PUERTO_MEMORIA = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        break;
    case DIALFS:
        log_debug(debug_logger, "Interfaz dialfs");
        TIEMPO_UNIDAD_TRABAJO = config_get_int_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        IP_MEMORIA = config_get_string_or_exit(config, "IP_MEMORIA");
        PUERTO_MEMORIA = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        PATH_BASE_DIALFS = config_get_string_or_exit(config, "PATH_BASE_DIALFS");
        BLOCK_SIZE = config_get_int_or_exit(config, "BLOCK_SIZE");
        BLOCK_COUNT = config_get_int_or_exit(config, "BLOCK_COUNT");
        RETRASO_COMPACTACION = config_get_int_or_exit(config, "RETRASO_COMPACTACION");
    }
}

t_tipo_interfaz parsear_a_t_tipo_interfaz(char *str)
{
    if (strcmp(str, "GENERICA") == 0) {
        return GENERICA;
    }
    if (strcmp(str, "STDIN") == 0) {
        return STDIN;
    }
    if (strcmp(str, "STDOUT") == 0) {
        return STDOUT;
    }
    if (strcmp(str, "DIALFS") == 0) {
        return DIALFS;
    }
    log_error(debug_logger, "Tipo de interfaz invalida");
    abort();
}
