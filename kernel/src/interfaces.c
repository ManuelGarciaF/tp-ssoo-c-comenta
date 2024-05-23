#include "main.h"

// Definiciones locales
static t_interfaz *registrar_interfaz(int conexion_io);
static int enviar_operacion(t_bloqueado_io *pb, int conexion_io);
static int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io);

static bool socket_sigue_abierto(int conexion);
static void eliminar_procesos_bloqueados(t_squeue *bloqueados);

// El hilo principal de cada interfaz
void *atender_io(void *param)
{
    int *conexion_io = (int *)param;

    recibir_handshake(*conexion_io);

    t_interfaz *self = registrar_interfaz(*conexion_io);

    log_info(debug_logger, "Se registro la interfaz \'%s\', con tipo %d\n", self->nombre, self->tipo);

    while (true) {
        // Esperamos que haya un proceso esperando.
        sem_wait(&(self->procesos_esperando));

        // Agarramos el primer elemento bloqueado, dejandolo en la cola para facilitar logs.
        t_bloqueado_io *proceso_bloqueado = squeue_peek(self->bloqueados);

        // Checkear que enviar funciona y que el socket sigue abierto
        if (enviar_operacion(proceso_bloqueado, *conexion_io) <= 0 || !socket_sigue_abierto(*conexion_io)) {
            log_error(debug_logger, "La interfaz %s se desconecto", self->nombre);
            break;
        }

        // Esperar la respuesta de la interfaz y ver que sea correcta
        if (recibir_int(*conexion_io) != MENSAJE_FIN_IO) {
            log_error(debug_logger, "La interfaz %s retorno un valor incorrecto, desconectandola", self->nombre);
            break;
        }

        // Tomar el permiso para agregar procesos a ready
        sem_wait(&sem_entrada_a_ready);
        // TODO checkear que luego de esta espera el proceso en new por el que se esperaba siga siendo el
        // mismo (puede haber sido eliminado por comando)

        // Sacamos el elemento de la cola (ya tenemos su referencia por peek)
        squeue_pop(self->bloqueados);

        // Volver a agregar el proceso a READY
        squeue_push(cola_ready, proceso_bloqueado->pcb);

        // Liberar el permiso para agregar procesos a ready
        sem_post(&sem_entrada_a_ready);

        sem_post(&sem_elementos_en_ready);

        // Liberar proceso_bloqueado
        list_destroy_and_destroy_elements(proceso_bloqueado->operacion, free);
        free(proceso_bloqueado);
    }

    // Si la interfaz se desconecto con procesos aun bloqueados, enviarlos a exit
    eliminar_procesos_bloqueados(self->bloqueados);

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

static int enviar_operacion(t_bloqueado_io *pb, int conexion_io)
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

static int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io)
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

// El socket puede haber cerrado aunque send() haya retornado > 0 :')
static bool socket_sigue_abierto(int conexion)
{
    int err = 0;
    socklen_t errlen = sizeof(err);
    if (getsockopt(conexion, SOL_SOCKET, SO_ERROR, (void *)&err, &errlen) == -1) {
        perror("getsockopt");
        return false;
    }

    if (err != 0) {
        log_error(debug_logger, "El socket de la interfaz cerro con error: %s", strerror(err));
        return false;
    }

    return true;
}

static void eliminar_procesos_bloqueados(t_squeue *bloqueados)
{
    while (!squeue_is_empty(bloqueados)) {
        t_bloqueado_io *pb = squeue_pop(bloqueados);
        eliminar_proceso(pb->pcb);

        // Liberar pb
        list_destroy_and_destroy_elements(pb->operacion, free);
        free(pb);
    }
}
