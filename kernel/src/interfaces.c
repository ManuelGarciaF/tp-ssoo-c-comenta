#include "main.h"

// Definiciones locales
static t_interfaz *registrar_interfaz(int conexion_io);
static int enviar_operacion(t_bloqueado_io *pb, int conexion_io);
static int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io);

// El hilo principal de cada interfaz
void *atender_io(void *param)
{
    int *conexion_io = (int *)param;

    recibir_handshake(*conexion_io);

    t_interfaz *self = registrar_interfaz(*conexion_io);

    log_info(debug_logger, "Se registro la interfaz \'%s\', con tipo %d\n", self->nombre, self->tipo);

    // TODO poder romper el loop cuando se desconecta
    while (true) {
        // Esperamos que haya un proceso esperando.
        sem_wait(&(self->procesos_esperando));

        // Agarramos el primer elemento bloqueado, dejandolo en la cola para facilitar logs.
        t_bloqueado_io *proceso_bloqueado = squeue_peek(self->bloqueados);

        // Si fallo enviar, significa que la interfaz se desconecto
        if (enviar_operacion(proceso_bloqueado, *conexion_io) < 0) {
            log_error(debug_logger, "La interfaz %s se desconecto", self->nombre);
            break;
        }

        // Esperar la respuesta de la interfaz y ver que sea correcta
        if (recibir_int(*conexion_io) != MENSAJE_FIN_IO) {
            log_error(debug_logger, "La interfaz %s retorno un valor incorrecto, desconectandola", self->nombre);
            break;
        }

        // Sacamos el elemento de la cola
        squeue_pop(self->bloqueados);

        // Volver a agregar el proceso a READY
        squeue_push(cola_ready, proceso_bloqueado->pcb);
        sem_post(&sem_elementos_en_ready);

        // Liberar proceso_bloqueado
        list_destroy_and_destroy_elements(proceso_bloqueado->operacion, free);
        free(proceso_bloqueado);
    }

    // Si la interfaz se desconecto con procesos aun bloqueados, enviarlos a exit


    // Limpiar datos interfaz
    sdictionary_remove(interfaces_conectadas, self->nombre);
    free(self->nombre);
    sem_destroy(&(self->procesos_esperando));
    squeue_destroy(self->bloqueados);
    free(self);

    close(*conexion_io);
    free(conexion_io);
    pthread_exit(NULL);
}

bool existe_interfaz(char *nombre)
{
    return sdictionary_has_key(interfaces_conectadas, nombre);
}

bool interfaz_soporta_operacion(char *nombre, t_operacion_io op)
{
    assert(existe_interfaz(nombre));

    t_tipo_interfaz tipo_requerido = TIPO_INTERFAZ_CAPAZ_DE_HACER[op];

    t_interfaz *interfaz = sdictionary_get(interfaces_conectadas, nombre);

    return interfaz->tipo == tipo_requerido;
}

static t_interfaz *registrar_interfaz(int conexion_io)
{
    // Recibir la info de la interfaz
    t_list *paquete_info_interfaz = recibir_paquete(conexion_io);
    char *nombre_interfaz = list_get(paquete_info_interfaz, 0);
    t_tipo_interfaz *tipo_interfaz = list_get(paquete_info_interfaz, 1);

    // Crear el t_interfaz
    t_interfaz *interfaz = malloc(sizeof(t_interfaz));
    interfaz->nombre = malloc(strlen(nombre_interfaz) + 1);
    strcpy(interfaz->nombre, nombre_interfaz);
    interfaz->tipo = *tipo_interfaz;
    interfaz->conexion = conexion_io;
    interfaz->bloqueados = squeue_create();
    sem_init(&(interfaz->procesos_esperando), 0, 0); // Comienza en 0

    // Guardarlo en el diccionario.
    sdictionary_put(interfaces_conectadas, interfaz->nombre, interfaz);

    list_destroy_and_destroy_elements(paquete_info_interfaz, free);

    return interfaz;
}

int enviar_operacion(t_bloqueado_io *pb, int conexion_io)
{
    switch (pb->opcode) {
    case GEN_SLEEP:
        return enviar_gen_sleep(pb, conexion_io);
    case STDIN_READ:
    case STDOUT_WRITE:
    case FS_CREATE:
    case FS_DELETE:
    case FS_TRUNCATE:
    case FS_WRITE:
    case FS_READ:
        assert(false && "Not implemented");
    }
    assert(false && "Operacion invalida");
}

int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = GEN_SLEEP;
    agregar_a_paquete(paquete, &op, sizeof(int));
    uint32_t *unidades_trabajo = list_get(pb->operacion, 2); // Es el 3er elemento, el 1er parametro.
    agregar_a_paquete(paquete, unidades_trabajo, sizeof(uint32_t));
    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}
