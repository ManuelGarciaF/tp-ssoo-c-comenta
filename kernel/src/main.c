#include "./main.h"

/*
** Variables Globales
*/
t_log *debug_logger;
t_log *kernel_logger;
t_config *config;

// Variables de config
char *PUERTO_ESCUCHA;
char *IP_MEMORIA;
char *PUERTO_MEMORIA;
char *IP_CPU;
char *PUERTO_CPU_DISPATCH;
char *PUERTO_CPU_INTERRUPT;
int grado_multiprogramacion;
int QUANTUM;
t_algoritmo_planificacion ALGORITMO_PLANIFICACION;

int pid_en_ejecucion = -1; // -1 Cuando no hay ninguno
int procesos_extra_multiprogramacion = 0;

int conexion_memoria;
pthread_mutex_t mutex_conexion_memoria;
int conexion_cpu_dispatch;
int conexion_cpu_interrupt;

t_squeue *cola_new;
t_squeue *cola_ready;
t_sdictionary *colas_blocked_recursos;
t_slist *nombres_interfaces;
t_squeue *cola_exit;

t_sdictionary *interfaces_conectadas;

t_sdictionary *instancias_recursos;
t_slist *asignaciones_recursos;
t_slist *nombres_recursos;

// Semaforos
sem_t sem_multiprogramacion;
sem_t sem_elementos_en_new;
sem_t sem_elementos_en_ready;
sem_t sem_interrupcion_rr;

sem_t sem_entrada_a_ready;
sem_t sem_entrada_a_exec;
sem_t sem_manejo_desalojo_cpu;
bool planificacion_pausada; // Para registrar el estado de la planificacion.

// Funciones locales
static void inicializar_globales(void);
static void *esperar_conexiones_io(void *);
static t_algoritmo_planificacion parse_algoritmo_planifiacion(const char *str);

int main(int argc, char *argv[])
{
    inicializar_globales();

    // Conexion con el cpu
    conexion_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if (!realizar_handshake(conexion_cpu_dispatch)) {
        log_error(debug_logger, "No Se pudo realizar un handshake con el CPU (dispatch)");
    }

    conexion_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (!realizar_handshake(conexion_cpu_interrupt)) {
        log_error(debug_logger, "No se pudo realizar un handshake con el CPU (interrupt)");
    }

    // Conexion con la memoria
    pthread_mutex_lock(&mutex_conexion_memoria);
    conexion_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (!realizar_handshake(conexion_memoria)) {
        log_error(debug_logger, "No se pudo realizar un handshake con la memoria");
    }
    // Avisar quien es a la memoria
    enviar_int(MENSAJE_A_MEMORIA_KERNEL, conexion_memoria);
    pthread_mutex_unlock(&mutex_conexion_memoria);

    // Crear hilo para esperar conexiones de entrada/salida
    pthread_t hilo_esperar_io;
    int iret = pthread_create(&hilo_esperar_io, NULL, esperar_conexiones_io, NULL);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para esperar conexiones");
    }

    // Iniciar planificadores
    pthread_t hilo_planificador_largo_plazo;
    iret = pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo, NULL);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el planificador de largo plazo");
    }

    pthread_t hilo_planificador_corto_plazo;
    iret = pthread_create(&hilo_planificador_corto_plazo, NULL, planificador_corto_plazo, NULL);
    if (iret != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el planificador de corto plazo");
    }

    // Darle el hilo principal a la consola
    correr_consola();

    close(conexion_cpu_dispatch);
    close(conexion_cpu_interrupt);
    close(conexion_memoria);

    // No me molesto en liberar las variables globales.

    pthread_exit(NULL);
}

static void inicializar_globales(void)
{
    debug_logger = log_create("kernel_debug.log", "debug", true, LOG_LEVEL_DEBUG);
    kernel_logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);

    config = config_create("kernel.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    PUERTO_ESCUCHA = config_get_string_or_exit(config, "PUERTO_ESCUCHA");
    IP_MEMORIA = config_get_string_or_exit(config, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    IP_CPU = config_get_string_or_exit(config, "IP_CPU");
    PUERTO_CPU_DISPATCH = config_get_string_or_exit(config, "PUERTO_CPU_DISPATCH");
    PUERTO_CPU_INTERRUPT = config_get_string_or_exit(config, "PUERTO_CPU_INTERRUPT");
    grado_multiprogramacion = config_get_int_or_exit(config, "GRADO_MULTIPROGRAMACION");
    ALGORITMO_PLANIFICACION =
        parse_algoritmo_planifiacion(config_get_string_or_exit(config, "ALGORITMO_PLANIFICACION"));
    QUANTUM = config_get_int_or_exit(config, "QUANTUM");

    // sem_multiprogramacion es el numero de procesos nuevos que se pueden crear,
    // comienza en el grado de multiprogramacion.
    sem_init(&sem_multiprogramacion, 0, grado_multiprogramacion);
    // sem_elementos_en_new permite bloquear el plp hasta que se lo necesita,
    // contiene el numero de elementos en cola_new.
    sem_init(&sem_elementos_en_new, 0, 0);

    // Contiene el numero de elementos en cola_ready.
    sem_init(&sem_elementos_en_ready, 0, 0);

    cola_new = squeue_create();
    cola_ready = squeue_create();
    colas_blocked_recursos = sdictionary_create();
    cola_exit = squeue_create();

    interfaces_conectadas = sdictionary_create();
    nombres_interfaces = slist_create();

    instancias_recursos = sdictionary_create();
    asignaciones_recursos = slist_create();
    nombres_recursos = slist_create();

    char **recursos_strs = config_get_array_value(config, "RECURSOS");
    char **instancias_recursos_strs = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    for (int i = 0; recursos_strs[i] != NULL; i++) {
        // Guardar los recursos disponibles.
        int *instancias = malloc(sizeof(int));
        *instancias = atoi(instancias_recursos_strs[i]);
        sdictionary_put(instancias_recursos, recursos_strs[i], instancias);

        // Inicializar las colas de bloqueados para cada recurso.
        t_squeue *cola_bloqueados = squeue_create();
        sdictionary_put(colas_blocked_recursos, recursos_strs[i], cola_bloqueados);

        // Guardar nombres en nombes_recursos
        slist_add(nombres_recursos, string_duplicate(recursos_strs[i]));
    }

    string_array_destroy(recursos_strs);
    string_array_destroy(instancias_recursos_strs);

    // Comienzan en 1 ya que la planificacion inicialmente esta habilitada.
    sem_init(&sem_entrada_a_ready, 0, 1);
    sem_init(&sem_entrada_a_exec, 0, 1);
    sem_init(&sem_manejo_desalojo_cpu, 0, 1);
    sem_init(&sem_interrupcion_rr, 0, 1);
    planificacion_pausada = false;

    pthread_mutex_init(&mutex_conexion_memoria, NULL);
}

static void *esperar_conexiones_io(void *param)
{
    // Esperar constantemente conexiones de io
    int socket_escucha = iniciar_servidor(PUERTO_ESCUCHA);
    while (true) {
        // Guardo el socket en el heap para no perderlo
        int *conexion_io = malloc(sizeof(int));
        *conexion_io = esperar_cliente(socket_escucha);

        // Crear hilo para manejar esta conexion
        pthread_t hilo;
        int iret = pthread_create(&hilo, NULL, atender_io, conexion_io);
        if (iret != 0) {
            log_error(debug_logger, "No se pudo crear un hilo para atender la interfaz de I/O");
            exit(1);
        }
        pthread_detach(hilo);
    }
}

static t_algoritmo_planificacion parse_algoritmo_planifiacion(const char *str)
{
    if (!strcmp(str, "FIFO")) {
        return FIFO;
    } else if (!strcmp(str, "RR")) {
        return RR;
    } else if (!strcmp(str, "VRR")) {
        return VRR;
    }
    log_error(debug_logger, "El algoritmo de planificacion en el config no es valido");
    exit(1);
}

void pausar_planificacion(void)
{
    if (planificacion_pausada) {
        log_warning(debug_logger, "Se intento pausar la planificacion cuando ya esta pausada, ignorando la solicitud.");
        return;
    }

    // Retener el permiso para que procesos entren a ready, exec o vuelvan de la cpu.
    // Va a esperar que terminen las operaciones actuales de los planificadores antes
    // de bloquear.
    sem_wait(&sem_entrada_a_ready);
    sem_wait(&sem_entrada_a_exec);
    sem_wait(&sem_manejo_desalojo_cpu);
    sem_wait(&sem_interrupcion_rr); // Tambien no permitir que se envien interrupciones por fin de Quantum
    planificacion_pausada = true;
}

void reanudar_planificacion(void)
{
    if (!planificacion_pausada) {
        log_warning(debug_logger,
                    "Se intento reanudar la planificacion cuando no se encuentra pausada, ignorando la solicitud.");
        return;
    }

    // Devolver el permiso para que los procesos se muevan
    sem_post(&sem_entrada_a_ready);
    sem_post(&sem_entrada_a_exec);
    sem_post(&sem_manejo_desalojo_cpu);
    sem_post(&sem_interrupcion_rr);
    planificacion_pausada = false;
}

void eliminar_proceso(t_pcb *pcb)
{
    // Si se achico el grado_multiprogramacion, los proximos procesos que
    // se eliminen no liberan espacio en el sem_multiprogramacion.
    if (procesos_extra_multiprogramacion > 0) {
        procesos_extra_multiprogramacion--;
    } else {
        sem_post(&sem_multiprogramacion);
    }

    // Avisar a memoria que debe liberar el proceso.
    pthread_mutex_lock(&mutex_conexion_memoria);
    enviar_int(OPCODE_FIN_PROCESO, conexion_memoria);
    enviar_int(pcb->pid, conexion_memoria);
    pthread_mutex_unlock(&mutex_conexion_memoria);

    liberar_asignaciones_recurso(pcb->pid);

    agregar_a_exit(pcb->pid);

    pcb_destroy(pcb);
}

void log_cola_ready(void)
{
    char *lista_pids = obtener_lista_pids_pcb(cola_ready);
    log_info(kernel_logger, "Cola Ready READY: [%s]", lista_pids);
    free(lista_pids);
    // TODO agregar READY+ cuando este hecho VRR
}

void agregar_a_exit(uint32_t pid_val)
{
    uint32_t *pid = malloc(sizeof(uint32_t));
    *pid = pid_val;
    squeue_push(cola_exit, pid);
}
