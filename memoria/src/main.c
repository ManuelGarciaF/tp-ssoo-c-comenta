#include "./main.h"

// Variables globales
t_log *debug_logger;
t_log *memoria_logger;
t_config *config;

char *PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
char *PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

t_sdictionary *procesos;
void *memoria_de_usuario;
pthread_mutex_t mutex_memoria_de_usuario;

int num_marcos;
t_bitarray *bitmap_marcos;
pthread_mutex_t mutex_bitmap_marcos;

// Funciones locales
static void inicializar_globales(void);
static void *atender_conexion(void *param);

int main(void)
{
    inicializar_globales();

    // Esperar conexiones.
    int socket_escucha = iniciar_servidor(PUERTO_ESCUCHA);
    while (true) {
        // Guardo el socket en el heap para no perderlo
        int *conexion = malloc(sizeof(int));
        *conexion = esperar_cliente(socket_escucha);

        // Crear hilo para manejar esta conexion
        pthread_t hilo;
        int iret = pthread_create(&hilo, NULL, atender_conexion, conexion);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para atender la conexion");
            exit(1);
        }
        pthread_detach(hilo);
    }
}

static void inicializar_globales(void)
{
    // Logs
    debug_logger = log_create("memoria_debug.log", "debug", true, LOG_LEVEL_DEBUG);
    memoria_logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);

    // Config
    config = config_create("memoria.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }
    // Variables de config
    PUERTO_ESCUCHA = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_int_or_exit(config, "TAM_MEMORIA");
    TAM_PAGINA = config_get_int_or_exit(config, "TAM_PAGINA");
    PATH_INSTRUCCIONES = config_get_string_or_exit(config, "PATH_INSTRUCCIONES");
    RETARDO_RESPUESTA = config_get_int_or_exit(config, "RETARDO_RESPUESTA");

    // El tamanio de memoria debe ser un multiplo del tamanio de pagina
    assert(TAM_MEMORIA % TAM_PAGINA == 0);
    num_marcos = TAM_MEMORIA / TAM_PAGINA;

    // Diccionario con pseudocodigo de procesos
    procesos = sdictionary_create();

    // Inicializar memoria de usuario
    memoria_de_usuario = malloc(TAM_MEMORIA);
    assert(memoria_de_usuario != NULL);
    // Settear la memoria a 0
    memset(memoria_de_usuario, 0, TAM_MEMORIA);
    pthread_mutex_init(&mutex_memoria_de_usuario, NULL);

    // Inicializar bitmap
    int num_bytes = ceil_div(num_marcos, 8);
    void *bitarray = malloc(num_bytes);
    assert(bitarray != NULL);
    // Settear la memoria a 0
    memset(bitarray, 0, num_bytes);
    bitmap_marcos = bitarray_create_with_mode(bitarray, num_bytes, LSB_FIRST);
    pthread_mutex_init(&mutex_bitmap_marcos, NULL);
}

static void *atender_conexion(void *param)
{
    int *socket_conexion = param;
    recibir_handshake(*socket_conexion);

    t_mensaje_identificacion_memoria modulo = recibir_int(*socket_conexion);

    switch (modulo) {
    case MENSAJE_A_MEMORIA_CPU:
        atender_cpu(*socket_conexion);
        break;
    case MENSAJE_A_MEMORIA_KERNEL:
        atender_kernel(*socket_conexion);
        break;
    case MENSAJE_A_MEMORIA_IO:
        atender_io(*socket_conexion);
        break;
    default:
        log_error(debug_logger, "El cliente no informo su identidad correctamente");
        break;
    }

    close(*socket_conexion);
    free(socket_conexion);
    pthread_exit(NULL);
}
