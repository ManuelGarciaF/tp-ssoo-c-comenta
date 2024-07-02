#include "main.h"
#include "utils.h"
#include "utils/bloque.h"

#include <assert.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <utils/sockets.h>
#include <utils/utils.h>

#define NOMBRE_MAX_LEN 512

//
// Estructuras
//

typedef struct {
    char nombre[NOMBRE_MAX_LEN];
    size_t bloque_inicial;
    size_t tam_bytes;
} t_metadata;

//
// Variables globales
//

static void *bloques = NULL;
static t_bitarray *bitmap = NULL; // 1 = ocupado, 0 = libre

//
// Funciones locales
//

static void create_file(char *nombre_archivo);
static void delete_file(char *nombre_archivo);
static void truncate_file(uint32_t pid, char *nombre_archivo, size_t tamanio_nuevo);
static void write_file(t_list *paquete, int conexion_memoria);
static void read_file(t_list *paquete, int conexion_memoria);

static void inicializar_dialfs(void);
static t_list *cargar_archivos_metadata(void);
static bool cmp_bloque_inicial_metadata(void *m1, void *m2);
static t_metadata leer_metadata(char *nombre_archivo);
static void actualizar_metadata(t_metadata metadata);
static size_t encontrar_primer_bloque_libre(void);
static char *obtener_path_archivo(char *nombre);
static size_t bloques_contiguos_disponibles(t_metadata metadata);
static void *leer_archivo_entero(t_metadata metadata);
static t_metadata compactar(char *nombre_archivo_a_mover);
static void sync_files(void);
static void mover_archivo(t_metadata metadata, size_t nuevo_bloque_inicial);

static void debug_print_archivo(char *nombre_archivo);

static inline size_t tam_bloques(size_t bytes);
static inline void *puntero_bloque(size_t numero_bloque);
static inline void *puntero_dir_archivo(t_metadata metadata, size_t dir_archivo);

//
// Funcion principal
//
void manejar_dialfs(int conexion_kernel, int conexion_memoria)
{
    inicializar_dialfs();

    while (true) {
        bool err = false;
        t_list *paquete = recibir_paquete(conexion_kernel, &err);
        if (err) {
            log_error(debug_logger, "Hubo un error en la conexion con kernel");
            abort();
        }

        uint32_t *pid = list_get(paquete, 0);
        t_operacion_io *instruccion = list_get(paquete, 1);
        char *nombre_archivo = list_get(paquete, 2);
        assert(strlen(nombre_archivo) < NOMBRE_MAX_LEN); // Por las dudas

        switch (*instruccion) {
        case FS_CREATE: {
            create_file(nombre_archivo);
            log_info(entradasalida_logger, "PID: %u - Operacion: FS_CREATE", *pid);
            log_info(entradasalida_logger, "PID: %u - Crear Archivo: %s", *pid, nombre_archivo);
            break;
        }
        case FS_DELETE: {
            delete_file(nombre_archivo);
            log_info(entradasalida_logger, "PID: %u - Operacion: FS_DELETE", *pid);
            log_info(entradasalida_logger, "PID: %u - Eliminar Archivo: %s", *pid, nombre_archivo);
            break;
        }
        case FS_TRUNCATE: {
            size_t *tamanio_nuevo = list_get(paquete, 3);
            truncate_file(*pid, nombre_archivo, *tamanio_nuevo);
            log_info(entradasalida_logger, "PID: %u - Operacion: FS_TRUNCATE", *pid);
            log_info(entradasalida_logger,
                     "PID: %u - Truncar Archivo: %s - Tamaño: %zu",
                     *pid,
                     nombre_archivo,
                     *tamanio_nuevo);
            break;
        }
        case FS_WRITE: {
            // Logs dentro de la funcion.
            write_file(paquete, conexion_memoria);
            break;
        }
        case FS_READ: {
            // Logs dentro de la funcion.
            read_file(paquete, conexion_memoria);
            break;
        }
        default: {
            log_error(debug_logger, "Operacion invalida");
            abort();
            break;
        }
        }

        list_destroy_and_destroy_elements(paquete, free);

        enviar_int(MENSAJE_FIN_IO, conexion_kernel); // Avisar que terminamos
    }
}

static void create_file(char *nombre_archivo)
{
    size_t bloque_inicial = encontrar_primer_bloque_libre();

    // Creamos un config vacio
    t_config *config = malloc(sizeof(t_config));
    config->path = NULL;
    config->properties = dictionary_create();

    char *str_bloque_inicial = string_itoa(bloque_inicial);
    config_set_value(config, "BLOQUE_INICIAL", str_bloque_inicial);
    config_set_value(config, "TAMANIO_ARCHIVO", "0");
    free(str_bloque_inicial);

    // Guardamos el metadata en disco
    char *path = obtener_path_archivo(nombre_archivo);
    config_save_in_file(config, path);
    config_destroy(config);
    free(path);

    log_debug(debug_logger, "Creando el archivo %s en el bloque %zu", nombre_archivo, bloque_inicial);

    // Marcar el bloque inicial como usado
    bitarray_set_bit(bitmap, bloque_inicial);
    sync_files();
}

static void delete_file(char *nombre_archivo)
{
    // Sacar el archivo del diccionario.
    t_metadata metadata = leer_metadata(nombre_archivo);

    // Eliminar el archivo.
    char *path = obtener_path_archivo(nombre_archivo);
    int ret = remove(path);
    if (ret != 0) {
        log_error(debug_logger, "No se pudo eliminar el archivo %s", path);
    }
    free(path);

    // Marcar los bloques como libres en el bitmap
    bitarray_clean_range(bitmap,
                         metadata.bloque_inicial,
                         metadata.bloque_inicial + tam_bloques(metadata.bloque_inicial) - 1);
    sync_files();
}

static void truncate_file(uint32_t pid, char *nombre_archivo, size_t tamanio_nuevo)
{
    t_metadata metadata = leer_metadata(nombre_archivo);
    if (metadata.tam_bytes == tamanio_nuevo) { // Si no cambia no hacer nada
        return;
    }

    size_t actual_cantidad_bloques = tam_bloques(metadata.tam_bytes);
    size_t nueva_cantidad_bloques = tam_bloques(tamanio_nuevo);
    size_t index_bloque_final_original = metadata.bloque_inicial + actual_cantidad_bloques - 1;
    size_t index_bloque_final_nuevo = metadata.bloque_inicial + nueva_cantidad_bloques - 1;

    // Si hay que achicar.
    if (nueva_cantidad_bloques < actual_cantidad_bloques) {
        // Solo hay que marcar los bloques libres.
        bitarray_clean_range(bitmap, index_bloque_final_nuevo + 1, index_bloque_final_original);

    } else if (nueva_cantidad_bloques > actual_cantidad_bloques) { // Hay que agrandar.
        // Si no hay suficientes bloques libres compactar.
        if (bloques_contiguos_disponibles(metadata) < nueva_cantidad_bloques) {
            log_info(entradasalida_logger, "PID: %u - Inicio Compactación.", pid);
            // La metadata cambia al compactar
            metadata = compactar(nombre_archivo);

            // Hay que recalcular, ya que el bloque inicial cambio
            index_bloque_final_original = metadata.bloque_inicial + actual_cantidad_bloques - 1;
            index_bloque_final_nuevo = metadata.bloque_inicial + nueva_cantidad_bloques - 1;

            usleep(RETRASO_COMPACTACION * 1000); // Retraso en milisegundos
            log_info(entradasalida_logger, "PID: %u - Fin Compactación.", pid);
        }

        // Marcar los bloques ocupados.
        bitarray_set_range(bitmap, index_bloque_final_original + 1, index_bloque_final_nuevo);
    }

    // Actualizar el tamanio
    metadata.tam_bytes = tamanio_nuevo;
    actualizar_metadata(metadata);
    sync_files();
}

static void write_file(t_list *paquete, int conexion_memoria)
{
    t_list_iterator *p_it = list_iterator_create(paquete);

    uint32_t *pid = list_iterator_next(p_it);
    list_iterator_next(p_it); // Ignorar operacion
    char *nombre_archivo = list_iterator_next(p_it);
    size_t *dir_inicio_archivo = list_iterator_next(p_it);
    size_t *tamanio_total = list_iterator_next(p_it);
    // Lo que sigue en el paquete son los bloques.

    log_info(entradasalida_logger,
             "PID: %u - Escribir Archivo: %s - Tamaño a Escribir: %zu - Puntero Archivo: %zu",
             *pid,
             nombre_archivo,
             *tamanio_total,
             *dir_inicio_archivo);

    t_metadata metadata = leer_metadata(nombre_archivo);

    size_t curr_pos = *dir_inicio_archivo;
    while (list_iterator_has_next(p_it)) {
        t_bloque *bloque = list_iterator_next(p_it);

        void *destino_datos = puntero_dir_archivo(metadata, curr_pos);
        void *datos = leer_bloque_de_memoria(*pid, *bloque, conexion_memoria);
        memcpy(destino_datos, datos, bloque->tamanio);

        // FIXME borrar despues
        char *hexstring = print_hex((void *)datos, bloque->tamanio);
        log_debug(debug_logger, "Bloque (%zu; %zu), datos: %s", bloque->base, bloque->tamanio, hexstring);
        free(hexstring);
        // FIXME borrar despues

        free(datos);

        curr_pos += bloque->tamanio;
    }
    list_iterator_destroy(p_it);

    // FIXME borrar despues
    debug_print_archivo(nombre_archivo);

    sync_files();
}

static void read_file(t_list *paquete, int conexion_memoria)
{
    t_list_iterator *p_it = list_iterator_create(paquete);

    uint32_t *pid = list_iterator_next(p_it);
    list_iterator_next(p_it); // Ignorar operacion
    char *nombre_archivo = list_iterator_next(p_it);
    size_t *dir_inicio_archivo = list_iterator_next(p_it);
    size_t *tamanio_total = list_iterator_next(p_it);
    // Lo que sigue en el paquete son los bloques.

    log_info(entradasalida_logger,
             "PID: %u - Leer Archivo: %s - Tamaño a Leer: %zu - Puntero Archivo: %zu",
             *pid,
             nombre_archivo,
             *tamanio_total,
             *dir_inicio_archivo);

    // FIXME borrar despues
    debug_print_archivo(nombre_archivo);

    t_metadata metadata = leer_metadata(nombre_archivo);

    size_t curr_pos = *dir_inicio_archivo;
    while (list_iterator_has_next(p_it)) {
        t_bloque *bloque = list_iterator_next(p_it);

        void *inicio_datos = puntero_dir_archivo(metadata, curr_pos);
        escribir_bloque_a_memoria(*pid, *bloque, inicio_datos, conexion_memoria);

        // FIXME borrar despues
        char *hexstring = print_hex((void *)inicio_datos, bloque->tamanio);
        log_debug(debug_logger, "Bloque (%zu; %zu), datos: %s", bloque->base, bloque->tamanio, hexstring);
        free(hexstring);
        // FIXME borrar despues

        curr_pos += bloque->tamanio;
    }
    list_iterator_destroy(p_it);

    sync_files();
}

static void inicializar_dialfs(void)
{
    // Abrir archivo de bloques.
    size_t tam_archivo_bloques = BLOCK_SIZE * BLOCK_COUNT;
    char *path_archivo_bloques = string_duplicate(PATH_BASE_DIALFS);
    string_append(&path_archivo_bloques, "/bloques.dat");

    int fd_bloques = open(path_archivo_bloques, O_RDWR | O_CREAT, 0644);
    if (fd_bloques == -1) {
        log_error(debug_logger, "No se pudo abrir o crear el archivo de bloques");
        abort();
    }
    free(path_archivo_bloques);

    // Truncarlo al tamaño correcto (en caso que lo estemos creando).
    int ret = ftruncate(fd_bloques, tam_archivo_bloques);
    if (ret == -1) {
        log_error(debug_logger, "No se pudo truncar el archivo de bloques");
        abort();
    }

    // Mapearlo a memoria.
    bloques = mmap(NULL, tam_archivo_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bloques, 0);
    if (bloques == MAP_FAILED) {
        log_error(debug_logger, "No se pudo mapear el archivo de bloques");
        abort();
    }

    // Abrir el archivo de bitmap.
    size_t tam_archivo_bitmap = ceil_div(BLOCK_COUNT, 8); // Un bit por bloque => 8 bloques por byte.
    char *path_archivo_bitmap = string_duplicate(PATH_BASE_DIALFS);
    string_append(&path_archivo_bitmap, "/bitmap.dat");

    int fd_bitmap = open(path_archivo_bitmap, O_RDWR | O_CREAT, 0644);
    if (fd_bitmap == -1) {
        log_error(debug_logger, "No se pudo abrir o crear el archivo del bitmap");
        abort();
    }
    free(path_archivo_bitmap);

    // Truncarlo al tamaño correcto.
    ret = ftruncate(fd_bitmap, tam_archivo_bitmap);
    if (ret == -1) {
        log_error(debug_logger, "No se pudo truncar el archivo del bitmap");
        abort();
    }

    // Mapearlo a memoria y wrappearlo en un t_bitarray.
    void *memoria_bitmap = mmap(NULL, tam_archivo_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);
    if (memoria_bitmap == MAP_FAILED) {
        log_error(debug_logger, "No se pudo mapear el archivo del bitmap");
        abort();
    }
    bitmap = bitarray_create_with_mode(memoria_bitmap, BLOCK_COUNT, LSB_FIRST);
}

// Devuelve una lista con t_metadata para cada archivo, ordenados por bloque inicial.
static t_list *cargar_archivos_metadata(void)
{
    t_list *list = list_create();

    DIR *dir = opendir(PATH_BASE_DIALFS);
    if (dir == NULL) {
        log_error(debug_logger, "No se pudo abrir el directorio del dialfs");
        abort();
    }

    // Iterar sobre los archivos en el directorio.
    struct dirent *entrada;
    while ((entrada = readdir(dir)) != NULL) {
        // Saltear subdirectorios, "bitmap.dat" y "bloques.dat".
        if (entrada->d_type != DT_REG || strcmp(entrada->d_name, "bitmap.dat") == 0 ||
            strcmp(entrada->d_name, "bloques.dat") == 0) {
            continue;
        }

        t_metadata metadata = leer_metadata(entrada->d_name);
        // Necesitamos guardarlo en el heap.
        t_metadata *p = malloc(sizeof(t_metadata));
        if (p == NULL) {
            log_error(debug_logger, "No se pudo alojar memoria");
            abort();
        }
        memcpy(p, &metadata, sizeof(t_metadata));

        list_add(list, p);
    }
    closedir(dir);

    // Ordenar los archivos por bloque inicial para facilitar la compactacion.
    list_sort(list, cmp_bloque_inicial_metadata);

    return list;
}

static bool cmp_bloque_inicial_metadata(void *m1, void *m2)
{
    // Si el primero tiene un bloque inicial anterior, va primero.
    return ((t_metadata *)m1)->bloque_inicial < ((t_metadata *)m2)->bloque_inicial;
}

static t_metadata leer_metadata(char *nombre_archivo)
{
    char *path = obtener_path_archivo(nombre_archivo);
    t_config *config = config_create(path);
    if (config == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    free(path);

    size_t bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");
    size_t tamanio_archivo = config_get_int_value(config, "TAMANIO_ARCHIVO");
    config_destroy(config);

    t_metadata metadata = {.bloque_inicial = bloque_inicial, .tam_bytes = tamanio_archivo};
    strcpy(metadata.nombre, nombre_archivo); // Guardar el nombre.

    return metadata;
}

static void actualizar_metadata(t_metadata metadata)
{
    char *path = obtener_path_archivo(metadata.nombre);
    t_config *config = config_create(path);
    free(path);

    char *str_bloque_inicial = string_itoa(metadata.bloque_inicial);
    config_set_value(config, "BLOQUE_INICIAL", str_bloque_inicial);
    free(str_bloque_inicial);

    char *str_tam_bytes = string_itoa(metadata.tam_bytes);
    config_set_value(config, "TAMANIO_ARCHIVO", str_tam_bytes);
    free(str_tam_bytes);

    config_save(config);
    config_destroy(config);
}

static size_t encontrar_primer_bloque_libre(void)
{
    size_t index = 0;
    while (index < (size_t)BLOCK_COUNT) {
        if (bitarray_test_bit(bitmap, index) == false) {
            break;
        }
        index++;
    }
    return index;
}

static char *obtener_path_archivo(char *nombre)
{
    char *file_path = string_duplicate(PATH_BASE_DIALFS);
    string_append(&file_path, "/");
    string_append(&file_path, nombre);
    return file_path;
}

// Devuelve los bloques vacios a partir de un punto
static size_t bloques_contiguos_disponibles(t_metadata metadata)
{
    size_t tam = tam_bloques(metadata.tam_bytes);
    size_t tam_max = tam;
    for (size_t i = metadata.bloque_inicial + tam; i < (size_t)BLOCK_COUNT; i++) {
        if (bitarray_test_bit(bitmap, i)) {
            break;
        }
        tam_max++;
    }

    return tam_max;
}

// Crea una copia de los contenidos de un archivo
static void *leer_archivo_entero(t_metadata metadata)
{
    void *contenido = malloc(metadata.tam_bytes);
    if (contenido == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    void *inicio_archivo = puntero_bloque(metadata.bloque_inicial);

    return memcpy(contenido, inicio_archivo, metadata.tam_bytes);
}

// Mueve el archivo pasado por parametro al final. Retorna la metadata actualizada del archivo movido.
static t_metadata compactar(char *nombre_archivo_a_mover)
{
    void *contenidos_archivo_a_mover = NULL;
    t_metadata m_archivo_a_mover = {0};

    // Recorremos el archivo de bloques con un puntero, para saber el proximo lugar vacio.
    size_t bloque_actual = 0;

    t_list *archivos = cargar_archivos_metadata(); // Los archivos ya estan ordenados.
    t_list_iterator *it = list_iterator_create(archivos);
    while (list_iterator_has_next(it)) {
        t_metadata *m = list_iterator_next(it);
        assert(m->bloque_inicial >= bloque_actual); // Checkear que si esten ordenados.

        // Si encontramos el archivo a mover.
        if (strcmp(m->nombre, nombre_archivo_a_mover) == 0) {
            // Guardarlo en memoria
            contenidos_archivo_a_mover = leer_archivo_entero(*m);
            m_archivo_a_mover = *m;

            // Sacarlo del bitmap
            size_t index_bloque_final = m->bloque_inicial + tam_bloques(m->tam_bytes) - 1;
            bitarray_clean_range(bitmap, m->bloque_inicial, index_bloque_final);

            continue; // No queremos incrementar el puntero.
        }

        // Mover el archivo si queda espacio vacio en el medio
        if (bloque_actual < m->bloque_inicial) {
            mover_archivo(*m, bloque_actual);
        }

        // Avanzar el puntero
        bloque_actual += tam_bloques(m->tam_bytes);
    }
    list_iterator_destroy(it);

    // Mover el archivo que sacamos al final.
    // Mover los datos.
    void *nueva_pos = puntero_bloque(bloque_actual);
    memcpy(nueva_pos, contenidos_archivo_a_mover, m_archivo_a_mover.tam_bytes);

    // Actualizar el bitmap.
    bitarray_set_range(bitmap, bloque_actual, bloque_actual + tam_bloques(m_archivo_a_mover.tam_bytes) - 1);

    // Actualizar la metadata.
    m_archivo_a_mover.bloque_inicial = bloque_actual;
    actualizar_metadata(m_archivo_a_mover);

    list_destroy_and_destroy_elements(archivos, free);
    free(contenidos_archivo_a_mover);

    sync_files();

    return m_archivo_a_mover;
}

// Asegura que los cambios a los archivos mapeados son guardados en disco.
static void sync_files(void)
{
    size_t tam_archivo_bloques = BLOCK_SIZE * BLOCK_COUNT;
    size_t tam_archivo_bitmap = ceil_div(BLOCK_COUNT, 8);

    msync(bloques, tam_archivo_bloques, MS_SYNC);
    msync(bitmap->bitarray, tam_archivo_bitmap, MS_SYNC);
}

static void mover_archivo(t_metadata metadata, size_t nuevo_bloque_inicial)
{
    log_debug(debug_logger,
              "Moviendo el archivo %s, bloque inicial %zu -> %zu",
              metadata.nombre,
              metadata.bloque_inicial,
              nuevo_bloque_inicial);

    // Mover los datos.
    void *nueva_pos = puntero_bloque(nuevo_bloque_inicial);
    void *actual_pos = puntero_bloque(metadata.bloque_inicial);
    memmove(nueva_pos, actual_pos, metadata.tam_bytes);

    // Actualizar el bitmap.
    size_t index_bloque_final_original = metadata.bloque_inicial + tam_bloques(metadata.tam_bytes) - 1;
    size_t index_bloque_final_nuevo = nuevo_bloque_inicial + tam_bloques(metadata.tam_bytes) - 1;
    bitarray_clean_range(bitmap, metadata.bloque_inicial, index_bloque_final_original);
    bitarray_set_range(bitmap, nuevo_bloque_inicial, index_bloque_final_nuevo);

    // Actualizar la metadata.
    metadata.bloque_inicial = nuevo_bloque_inicial;
    actualizar_metadata(metadata);
}

static void debug_print_archivo(char *nombre_archivo)
{
    char *contenido = leer_archivo_entero(leer_metadata(nombre_archivo));
    log_debug(debug_logger, "<Contenido de %s>: %s", nombre_archivo, contenido);
    free(contenido);
}

// Devuelve el numero de bloques que ocupa una cantidad de bytes
static inline size_t tam_bloques(size_t bytes)
{
    if (bytes == 0) {
        return 1; // Caso especial, un archivo de 0 bytes tambien ocupa 1 bloque
    }
    return ceil_div(bytes, BLOCK_SIZE);
}

// Devuelve un puntero a la posicion dentro del espacio de memoria de bloques, que corresponde al numero de bloque.
static inline void *puntero_bloque(size_t numero_bloque)
{
    return (char *)bloques + (numero_bloque * BLOCK_SIZE);
}

// Devuelve un puntero a la posicion dentro del espacio de memoria de bloques,
// que corresponde a la direccion dentro del archivo pasado.
static inline void *puntero_dir_archivo(t_metadata metadata, size_t dir_archivo)
{
    assert(dir_archivo < metadata.tam_bytes); // Asegurarse de que esta dentro del archivo
    return ((char *)puntero_bloque(metadata.bloque_inicial)) + dir_archivo;
}
