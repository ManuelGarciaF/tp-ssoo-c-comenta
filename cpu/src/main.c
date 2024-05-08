#include "./main.h"

/*
** Variables globales
*/
t_log *debug_logger;
t_log *cpu_logger;
t_pcb *pcb;

// Variables de config
char *ip_memoria;
char *puerto_memoria;
char *puerto_escucha_dispatch;
char *puerto_escucha_interrupt;


int main(int argc, char *argv[])
{
    debug_logger =
        log_create("cpu_debug.log", "cpu_debug", true, LOG_LEVEL_INFO);
    cpu_logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);

    t_config *config = config_create("cpu.config");
    if (config == NULL) {
        log_error(debug_logger, "No se pudo crear la config");
        exit(1);
    }
    cargar_config(config);

    int socket_escucha_dispatch = iniciar_servidor(puerto_escucha_dispatch);
    int socket_escucha_interrupt = iniciar_servidor(puerto_escucha_interrupt);

    pthread_t hilo_interrupt;
    // Crear hilo para interrupt
    if (pthread_create(&hilo_interrupt,
                       NULL,
                       (void *)servidor_interrupt,
                       &socket_escucha_interrupt) != 0) {
        log_error(debug_logger,
                  "No se pudo crear un hilo para el servidor de interrupt");
        exit(1);
    }

    // Conectar con la memoria
    int conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);

    if (!realizar_handshake(conexion_memoria)) {
        log_error(debug_logger, "No se pudo realizar un handshake con memoria");
    }
    enviar_mensaje(MENSAJE_A_MEMORIA_CPU, conexion_memoria);

    // Espera a que se conecte con el kernel y devuelve la conexion
    int conexion_dispatch = aceptar_conexion_kernel(socket_escucha_dispatch);

    while(true) {
        // Recibe PCB
        log_info(debug_logger, "Esperando PCB");
        pcb = pcb_receive(conexion_dispatch);
        log_info(debug_logger, "Recibido PCB exitosamente");

        // Fetch
        char *instruccion = fetch(pcb->pid, pcb->program_counter, conexion_memoria);

        // Decode
        decode(instruccion);
        free(instruccion);
    }

    // Esperar que los hilos terminen
    pthread_join(hilo_interrupt, NULL);

    // Cerrar sockets
    close(conexion_memoria);
    close(socket_escucha_dispatch);

    // Liberar memoria
    log_destroy(debug_logger);
    log_destroy(cpu_logger);
    config_destroy(config);

    // Esperar
    return 0;
}

/* Inicializa las variables globales */
void cargar_config(t_config *config)
{
    ip_memoria = config_get_string_or_exit(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_or_exit(config, "PUERTO_MEMORIA");
    puerto_escucha_dispatch =
        config_get_string_or_exit(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_escucha_interrupt =
        config_get_string_or_exit(config, "PUERTO_ESCUCHA_INTERRUPT");
}

int aceptar_conexion_kernel(int socket_escucha)
{
    int socket_conexion = esperar_cliente(socket_escucha);
    if (!recibir_handshake(socket_conexion)) {
        log_info(debug_logger,
                 "Hubo un error al intentar hacer el handshake");
        return -1;
    }
    return socket_conexion;
}

void *servidor_interrupt(int *socket_escucha)
{
    int socket_conexion = esperar_cliente(*socket_escucha);
    if (recibir_handshake(socket_conexion)) {
        log_info(debug_logger,
                 "Se realizo el handshake correctamente (interrupt)");
    }

    close(*socket_escucha);
    return NULL;
}


char *fetch(uint32_t pid, uint32_t program_counter, int conexion_memoria)
{
    enviar_mensaje(MENSAJE_SOLICITAR_INSTRUCCION, conexion_memoria);
    // Solcitiar instruccion
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &program_counter, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
    log_info(debug_logger, "Solicitada instruccion %d de proceso %d ", program_counter, pid);

    // Recibir instruccion
    return recibir_mensaje(conexion_memoria);
}
