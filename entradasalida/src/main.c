#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

int main(int argc, char* argv[]) {
    
    if(argc!=3){
        printf("Faltan operandos")
        print("\'Los operandos deben ser pasados como (1) nombre_interfaz (2) archivo_config\'")
        exit(1)
    }

    char *nombre_interfaz = argv[1];
    char *archivo_config = argv[2];
    
    debug_logger =
        log_create("entradasalida_debug.log", "entradasalida_debug", true, LOG_LEVEL_INFO);
    entradasalida_logger = log_create("entradasalida.log", "entradasalida", true, LOG_LEVEL_INFO);

    cargar_config(archivo_config);

    return 0;
}

void cargar_config(char* ruta)
{
    t_config *config = config_create(ruta);
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }
    
    // Propiedades Generales (van a estar en todos los tipos de interfaz)
    tipo_interfaz = parsear_a_t_tipo_interfaz(config_get_string_or_exit(config, "TIPO_INTERFAZ"));
    ip_kernel = config_get_string_or_exit(config, "IP_KERNEL");
    puerto_kernel = config_get_string_or_exit(config, "PUERTO_KERNEL");

    switch(tipo_interfaz){
        case GENERICA:
            tiempo_unidad_trabajo = config_get_string_or_exit(config, "TIEMPO_UNIDAD_TRABAJO");
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
            block_size = config_get_string_or_exit(config, "BLOCK_SIZE");
            block_count = config_get_string_or_exit(config, "BLOCK_COUNT");
    }

    config_destroy(config);
}

/*
    Esta funcion es creada para evitar un if anidado para el tipo_interfaz cuando se carga el config
    Ya que, como depende del tipo_interfaz y no se puede hacer un switch con strings de forma sencilla, la otra opcion seria ese if anidado
    Esto se da porque, suponemos que los distintos tipos de interfaz van a tener distintos datos en el archivo del config, por lo que si leo todos para todos va a tirar error
*/

t_tipo_interfaz parsear_a_t_tipo_interfaz(char* str){
    if(strcmp(str,"GENERICA")==0){
        return GENERICA;
    }
    if(strcmp(str,"STDIN")==0){
        return STDIN;
    }
    if(strcmp(str,"STDOUT")==0){
        return STDOUT;
    }
    if(strcmp(str,"DIALFS")==0){
        return DIALFS;
    }
}