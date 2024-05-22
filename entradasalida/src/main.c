#include "./main.h"

/*
** Variables globales
*/

t_log *debug_logger;
t_log *entradasalida_logger;

// Variables de config
t_tipo_interfaz tipo_interfaz;
char *nombre_interfaz;
int tiempo_unidad_trabajo;
char *ip_kernel;
char *puerto_kernel;
char *ip_memoria;
char *puerto_memoria;
char *path_base_dialfs;
int block_size;
int block_count;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Faltan operandos");
        printf("\'Los operandos deben ser pasados como (1) nombre_interfaz (2) "
               "archivo_config\'");
        exit(1);
    }

    nombre_interfaz = argv[1];
    char *archivo_config = argv[2];
    debug_logger = log_create("entradasalida_debug.log",
                              "entradasalida_debug",
                              true,
                              LOG_LEVEL_INFO);
    entradasalida_logger =
        log_create("entradasalida.log", "entradasalida", true, LOG_LEVEL_INFO);

    t_config *config = config_create(archivo_config);
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    cargar_config(config);

    // Conexion con el kernel
    int conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);
    if (!realizar_handshake(conexion_kernel)) {
        log_error(debug_logger,
                  "No se pudo realizar un handshake con el Kernel");
    }

    // Conexion con la memoria
    int conexion_memoria;
    if (tipo_interfaz != GENERICA ) {
         conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
        if (!realizar_handshake(conexion_memoria)) {
            log_error(debug_logger,
                    "No se pudo realizar un handshake con la memoria");
        }
        enviar_mensaje(MENSAJE_A_MEMORIA_IO, conexion_memoria);
    }

    t_paquete *paquete = crear_paquete();
    // Enviar nombre_interfaz y tipo_interfaz
    agregar_a_paquete(paquete, nombre_interfaz, strlen(nombre_interfaz) + 1);
    agregar_a_paquete(paquete, &tipo_interfaz, sizeof(t_tipo_interfaz));
    enviar_paquete(paquete, conexion_kernel);
    eliminar_paquete(paquete);

    switch (tipo_interfaz) {
    case GENERICA:
        generica(conexion_kernel);
        break;
    /*case STDIN:
        stdin();
        break;
    case STDOUT:
        stdout();
        break;
    case DIALFS:
        dialfs();
        break;
    */
    default:
        exit(1);
        // Unreachable
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
    tipo_interfaz = parsear_a_t_tipo_interfaz(
        config_get_string_or_exit(config, "TIPO_INTERFAZ"));
    ip_kernel = config_get_string_or_exit(config, "IP_KERNEL");
    puerto_kernel = config_get_string_or_exit(config, "PUERTO_KERNEL");

    switch (tipo_interfaz) {
    case GENERICA:
        tiempo_unidad_trabajo =
            config_get_int_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        break;
    case STDIN:
        ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
        puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        break;
    case STDOUT:
        tiempo_unidad_trabajo = config_get_string_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
        puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        break;
    case DIALFS:
        tiempo_unidad_trabajo = config_get_string_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
        ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
        puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
        path_base_dialfs = config_get_string_or_exit(config, "PATH_BASE_DIALFS");
        block_size = config_get_int_or_exit(config, "BLOCK_SIZE");
        block_count = config_get_int_or_exit(config, "BLOCK_COUNT");
    }
}

/*
    Esta funcion es creada para evitar un if anidado para el tipo_interfaz
   cuando se carga el config Ya que, como depende del tipo_interfaz y no se
   puede hacer un switch con strings de forma sencilla, la otra opcion seria ese
   if anidado Esto se da porque, suponemos que los distintos tipos de interfaz
   van a tener distintos datos en el archivo del config, por lo que si leo todos
   para todos va a tirar error
*/

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
    exit(1);
}
