#include "./main.h"

/*
** Variables globales
*/
t_log *debug_logger;
t_log *cpu_logger;
t_config *config;

t_pcb *pcb = NULL;
t_slist *interrupts;

uint32_t tam_pagina;

uint64_t num_instruccion_actual = 0;

// Variables de config
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;
int cantidad_entradas_tlb;
t_algoritmo_tlb algoritmo_tlb;

// Funciones locales
static void inicializar_globales(void);
static t_algoritmo_tlb parse_algoritmo_tlb(const char *str);

static void *servidor_interrupt(void *param);
static int aceptar_conexion_kernel(int socket_escucha);
static int conectar_a_memoria(void);


int main(int argc, char *argv[])
{
    inicializar_globales();
    inicializar_mmu();

    // Iniciar servidores
    int socket_escucha_dispatch = iniciar_servidor(puerto_escucha_dispatch);
    int socket_escucha_interrupt = iniciar_servidor(puerto_escucha_interrupt);

    // Crear hilo para interrupt
    pthread_t hilo_interrupt;
    if (pthread_create(&hilo_interrupt, NULL, servidor_interrupt, &socket_escucha_interrupt) != 0) {
        log_error(debug_logger, "No se pudo crear un hilo para el servidor de interrupt");
        exit(1);
    }

    int conexion_memoria = conectar_a_memoria();

    // Espera a que se conecte con el kernel y devuelve la conexion
    int conexion_dispatch = aceptar_conexion_kernel(socket_escucha_dispatch);


    // Ciclo de instruccion
    bool incrementar_pc;
    while (true) {
        // Por defecto siempre se incrementa.
        incrementar_pc = true;

        // Si no hay PCB, esperar uno.
        if (pcb == NULL) {
            log_info(debug_logger, "Esperando PCB");
            pcb = pcb_receive(conexion_dispatch);
            log_info(debug_logger, "Recibido PCB con PID: %u", pcb->pid);
        }

        /* pcb_debug_print(pcb); */

        // Fetch
        char *str_instruccion = fetch(pcb->pid, pcb->program_counter, conexion_memoria);
        log_info(cpu_logger, "PID: %d - FETCH - Program Counter: %d", pcb->pid, pcb->program_counter);

        // Decode
        t_instruccion instruccion = decode(str_instruccion);

        char **partes = string_n_split(str_instruccion, 2, " ");
        log_info(cpu_logger, "PID: %u - Ejecutando: %s - %s", pcb->pid, partes[0], partes[1]);
        string_array_destroy(partes);
        free(str_instruccion);

        // Excecute
        execute(instruccion, &incrementar_pc, conexion_dispatch);

        // Incrementamos el contador.
        num_instruccion_actual++;

        // El PCB pudo ser desalojado durante execute
        if (incrementar_pc && pcb != NULL) {
            pcb->program_counter++;
        }

        // Check interrupt
        check_interrupt(conexion_dispatch);
    }
}

static void inicializar_globales(void)
{
    debug_logger = log_create("cpu_debug.log", "debug", true, LOG_LEVEL_INFO);
    cpu_logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);

    config = config_create("cpu.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }

    // Leer variables de config
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch = config_get_string_or_exit(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt = config_get_string_or_exit(config, "PUERTO_ESCUCHA_INTERRUPT");
    cantidad_entradas_tlb = config_get_int_or_exit(config, "CANTIDAD_ENTRADAS_TLB");
    algoritmo_tlb = parse_algoritmo_tlb(config_get_string_or_exit(config, "ALGORITMO_TLB"));

    interrupts = slist_create();
}

static t_algoritmo_tlb parse_algoritmo_tlb(const char *str)
{
    if (!strcmp(str, "FIFO")) {
        return FIFO;
    } else if (!strcmp(str, "LRU")) {
        return LRU;
    }
    log_error(debug_logger, "El algoritmo de TLB en el config no es valido");
    exit(1);
}

static int aceptar_conexion_kernel(int socket_escucha)
{
    int socket_conexion = esperar_cliente(socket_escucha);
    if (!recibir_handshake(socket_conexion)) {
        log_info(debug_logger, "Hubo un error al intentar hacer el handshake con kernel");
        return -1;
    }
    return socket_conexion;
}

static int conectar_a_memoria(void)
{
    // Conectar con la memoria
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);

    if (!realizar_handshake(conexion_memoria)) {
        log_error(debug_logger, "No se pudo realizar un handshake con memoria");
    }
    enviar_int(MENSAJE_A_MEMORIA_CPU, conexion_memoria);

    // Recibir el tamanio de pagina.
    tam_pagina = recibir_int(conexion_memoria);

    return conexion_memoria;
}

static void *servidor_interrupt(void *param)
{
    int *socket_escucha_interrupt = param;
    int conexion_interrupt = aceptar_conexion_kernel(*socket_escucha_interrupt);

    while (true) {
        t_list *paquete = recibir_paquete(conexion_interrupt);
        uint32_t *pid_recibido = list_get(paquete, 0);
        t_motivo_desalojo *motivo_desalojo = list_get(paquete, 1);

        log_info(debug_logger,
                 "Se recibio una interrupcion para el pid %u con motivo %d",
                 *pid_recibido,
                 *motivo_desalojo);

        t_interrupcion *interrupcion = malloc(sizeof(t_interrupcion));
        interrupcion->pid = *pid_recibido;
        interrupcion->motivo = *motivo_desalojo;

        slist_add(interrupts, interrupcion);

        list_destroy_and_destroy_elements(paquete, free);
    }

    return NULL;
}

char *fetch(uint32_t pid, uint32_t program_counter, int conexion_memoria)
{
    enviar_int(OPCODE_SOLICITAR_INSTRUCCION, conexion_memoria);
    // Enviar PID y PC para solicitar una instruccion.
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &program_counter, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    // Recibir instruccion
    return recibir_str(conexion_memoria);
}

void devolver_pcb(t_motivo_desalojo motivo, int conexion_dispatch)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, pcb, sizeof(t_pcb));
    agregar_a_paquete(paquete, &motivo, sizeof(t_motivo_desalojo));
    enviar_paquete(paquete, conexion_dispatch);
    log_info(debug_logger, "PCB (PID: %u) devuelto con motivo %d", pcb->pid, motivo);
    eliminar_paquete(paquete);

    // Luego de enviarlo, lo liberamos
    pcb_destroy(pcb);
    pcb = NULL;
}

void check_interrupt(int conexion_dispatch)
{
    slist_lock(interrupts);

    // Si no estamos ejecutando, no tiene sentido ver si hay interrupts
    if (pcb != NULL) {
        t_list_iterator *it = list_iterator_create(interrupts->list);
        while (list_iterator_has_next(it)) {
            t_interrupcion *interrupcion = list_iterator_next(it);
            assert(interrupcion != NULL);
            assert(pcb != NULL);
            // Si hay un pid en la lista que coincida con el proceso en ejecucion
            if (interrupcion->pid == pcb->pid) {
                devolver_pcb(interrupcion->motivo, conexion_dispatch);
            }
        }
        list_iterator_destroy(it);
    }

    // Vaciar la lista
    list_clean_and_destroy_elements(interrupts->list, free);

    slist_unlock(interrupts);
}
