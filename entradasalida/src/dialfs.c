#include "main.h"

static void create(char *nombre_archivo);
static void delete(char *nombre_archivo);
static size_t devolver_primer_bloque_libre();
static void asignar_bloque_en_bitmap(size_t bloque);
static void liberar_bloque_en_bitmap(size_t bloque);

void manejar_dialfs(int conexion_kernel, int conexion_memoria)
{
    while (true) {
        t_list *paquete = recibir_paquete(conexion_kernel);
        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        char* nombre_archivo = list_get(paquete, 2);

        switch (*instruccion) {
        case FS_CREATE:
            create(nombre_archivo);
            log_info(entradasalida_logger, "PID: %u - Crear Archivo: %s", *pid, nombre_archivo);
            break;
        case FS_DELETE:
            delete(nombre_archivo);
            log_info(entradasalida_logger, "PID: %u - Eliminar Archivo: %s", *pid, nombre_archivo);
            break;
        case FS_TRUNCATE:
            // truncate();
            break;
        case FS_WRITE:
            // write();
            break;
        case FS_READ:
            // read();
            break;
        default:
            log_error(debug_logger, "Operacion invalida");
            list_destroy_and_destroy_elements(paquete, free);
            continue;
        }

        list_destroy_and_destroy_elements(paquete, free);
    }
    
}

void create(char *nombre_archivo)
{
    size_t bloque_inicial;
    //  bloque_inicial = devolver_primer_bloque_libre();
    
    // Creamos el config
    t_config *config = malloc(sizeof(t_config));

    config->path = NULL;
    config->properties = dictionary_create();
    
    config_set_value(config, "BLOQUE_INICIAL", string_itoa(bloque_inicial));
    config_set_value(config, "TAMANIO_ARCHIVO", "0");

    char *file_path = string_duplicate(PATH_BASE_DIALFS);
    string_append(&file_path, "/");
    string_append(&file_path, nombre_archivo);
    
    // Guardamos el metadata
    config_save_in_file(config, file_path);
    config_destroy(config);

    // asignar_bloque_en_bitmap(bloque_inicial);
}

void delete(char *nombre_archivo)
{
    char *file_path = string_duplicate(PATH_BASE_DIALFS);
    string_append(&file_path, "/");
    string_append(&file_path, nombre_archivo);
    
    t_config *config = config_create(file_path);
    size_t bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");
    config_destroy(config);
    
    remove(file_path);

    // liberar_bloque_en_bitmap(bloque_inicial);
}

static size_t devolver_primer_bloque_libre()
{
    // TODO
}

static void asignar_bloque_en_bitmap(size_t bloque)
{
    // TODO
}

static void liberar_bloque_en_bitmap(size_t bloque)
{
    // TODO
}