#include "main.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static void *bloques = NULL;
static t_bitarray *bitmap = NULL;

static void inicializar_dialfs(void);
static void create(char *nombre_archivo);
static void delete(char *nombre_archivo);
static size_t devolver_primer_bloque_libre(void);
static void asignar_bloque_en_bitmap(size_t bloque);
static void liberar_bloque_en_bitmap(size_t bloque);

void manejar_dialfs(int conexion_kernel, int conexion_memoria)
{
    inicializar_dialfs();

    while (true) {
        t_list *paquete = recibir_paquete(conexion_kernel);
        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        char *nombre_archivo = list_get(paquete, 2);

        switch (*instruccion) {
        case FS_CREATE:
            create(nombre_archivo);
            log_info(entradasalida_logger, "PID: %u - Crear Archivo: %s", *pid, nombre_archivo);
            break;
        case FS_DELETE:
            delete (nombre_archivo);
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

static void inicializar_dialfs(void)
{
    // Abrir archivo de bloques y mapearlo a memoria.
    size_t tam_archivo_bloques = BLOCK_SIZE * BLOCK_COUNT;
    char *path_archivo_bloques = string_duplicate(PATH_BASE_DIALFS);
    string_append(&path_archivo_bloques, "/bloques.dat");
    log_debug(debug_logger, "%s", path_archivo_bloques);
    int fd_bloques = open(path_archivo_bloques, O_RDWR | O_CREAT, 0644);
    assert(fd_bloques != -1);
    int ret = ftruncate(fd_bloques, tam_archivo_bloques);
    assert(ret != -1);
    bloques = mmap(NULL, tam_archivo_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);
    assert(bloques != MAP_FAILED);

    // Abrir el archivo de bitmap y cargarlo en un bitarray
    size_t tam_archivo_bitmap = ceil_div(BLOCK_COUNT, 8);
    char *path_archivo_bitmap = string_duplicate(PATH_BASE_DIALFS);
    string_append(&path_archivo_bitmap, "/bitmap.dat");
    int fd_bitmap = open(path_archivo_bitmap, O_RDWR | O_CREAT, 0644);
    assert(fd_bitmap != -1);
    ret = ftruncate(fd_bitmap, tam_archivo_bitmap);
    assert(ret != -1);
    void *memoria_bitmap = mmap(NULL, tam_archivo_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);
    assert(memoria_bitmap != MAP_FAILED);
    bitmap = bitarray_create_with_mode(memoria_bitmap, BLOCK_COUNT, LSB_FIRST);
}

void create(char *nombre_archivo)
{
    size_t bloque_inicial;
     bloque_inicial = devolver_primer_bloque_libre();

    // Creamos un config vacio
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

    asignar_bloque_en_bitmap(bloque_inicial);
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

    liberar_bloque_en_bitmap(bloque_inicial);
}

static size_t devolver_primer_bloque_libre(void)
{
    size_t index = 0;
    bool found = false;
    while (!found) {
        if (bitarray_test_bit(bitmap, index) == false) {
            found = true;
            break;
        }
        index++;
    }
    return index;
}

static void asignar_bloque_en_bitmap(size_t bloque)
{
    bitarray_set_bit(bitmap, bloque);
}

static void liberar_bloque_en_bitmap(size_t bloque)
{
    bitarray_clean_bit(bitmap, bloque);
}
