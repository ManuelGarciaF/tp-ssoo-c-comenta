#include "main.h"
#include "utils/sockets.h"
#include <commons/log.h>

// Definiciones locales
static t_interfaz *registrar_interfaz(int conexion_io);
static void desregistrar_interfaz(t_interfaz *interfaz);
static int enviar_operacion(t_bloqueado_io *pb, int conexion_io);
static int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io);
static int enviar_stdin_read(t_bloqueado_io *pb, int conexion_io);
static int enviar_stdout_write(t_bloqueado_io *pb, int conexion_io);
static int enviar_fs_create(t_bloqueado_io *pb, int conexion_io);
static int enviar_fs_delete(t_bloqueado_io *pb, int conexion_io);
static int enviar_fs_truncate(t_bloqueado_io *pb, int conexion_io);
static int enviar_fs_write(t_bloqueado_io *pb, int conexion_io);
static int enviar_fs_read(t_bloqueado_io *pb, int conexion_io);

static bool socket_sigue_abierto(int conexion);
static void eliminar_procesos_bloqueados(t_squeue *bloqueados);

// El hilo principal de cada interfaz
void *atender_io(void *param)
{
    int *conexion_io = (int *)param;

    recibir_handshake(*conexion_io);

    t_interfaz *self = registrar_interfaz(*conexion_io);

    log_info(debug_logger, "Se registro la interfaz \'%s\', de tipo %d", self->nombre, self->tipo);

    while (true) {
        // Esperamos que haya un proceso esperando.
        sem_wait(&(self->procesos_esperando));

        // Agarramos el primer elemento bloqueado, dejandolo en la cola.
        t_bloqueado_io *proceso_bloqueado = (squeue_is_empty(self->bloqueados)) ? NULL : squeue_peek(self->bloqueados);
        log_info(debug_logger, "PID %u solicitando op a %s", proceso_bloqueado->pcb->pid, self->nombre);

        // Checkear que enviar funciona y que el socket sigue abierto
        if (enviar_operacion(proceso_bloqueado, *conexion_io) <= 0 || !socket_sigue_abierto(*conexion_io)) {
            log_error(debug_logger, "La interfaz %s se desconecto", self->nombre);
            break;
        }

        // Esperar la respuesta de la interfaz y ver que sea correcta
        bool err = false;
        if (recibir_int(*conexion_io, &err) != MENSAJE_FIN_IO || err) {
            log_error(debug_logger, "La interfaz %s retorno un valor incorrecto, desconectandola", self->nombre);
            break;
        }
        log_info(debug_logger, "PID %u termino su solicitud a %s", proceso_bloqueado->pcb->pid, self->nombre);

        // Tomar el permiso para agregar procesos a ready
        sem_wait(&sem_entrada_a_ready);

        // Ver que mientras esperabamos no nos hayan sacado el proceso bloqueado (eliminado por consola)
        t_bloqueado_io *pb_nuevo = (squeue_is_empty(self->bloqueados)) ? NULL : squeue_peek(self->bloqueados);
        // Si cambio o no tenemos ningun proceso, seguimos con el proximo
        if (pb_nuevo != proceso_bloqueado || proceso_bloqueado == NULL || pb_nuevo == NULL) {
            // Liberar el permiso para agregar procesos a ready
            sem_post(&sem_entrada_a_ready);
            continue;
        }

        // Sacamos el elemento de la cola (ya tenemos su referencia por peek)
        squeue_pop(self->bloqueados);

        // Volver a agregar el proceso a READY
        // Si es VRR, lo agregamos a Ready+
        if (ALGORITMO_PLANIFICACION == VRR) {
            squeue_push(cola_ready_plus, proceso_bloqueado->pcb);
            log_info(kernel_logger,
                     "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY",
                     proceso_bloqueado->pcb->pid);
            log_cola_ready_plus();
        } else {
            squeue_push(cola_ready, proceso_bloqueado->pcb);
            log_info(kernel_logger,
                     "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY",
                     proceso_bloqueado->pcb->pid);
            log_cola_ready();
        }

        // Liberar el permiso para agregar procesos a ready
        sem_post(&sem_entrada_a_ready);

        sem_post(&sem_elementos_en_ready);

        // Liberar proceso_bloqueado (no el pcb)
        list_destroy_and_destroy_elements(proceso_bloqueado->operacion, free);
        free(proceso_bloqueado);
    }

    // Si la interfaz se desconecto con procesos aun bloqueados, enviarlos a exit
    eliminar_procesos_bloqueados(self->bloqueados);

    // Limpiar datos interfaz
    desregistrar_interfaz(self);

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
    bool err = false;
    t_list *paquete_info_interfaz = recibir_paquete(conexion_io, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexión con la interfaz");
        abort();
    }
    char *nombre_interfaz = list_get(paquete_info_interfaz, 0);
    t_tipo_interfaz *tipo_interfaz = list_get(paquete_info_interfaz, 1);

    // Crear el t_interfaz
    t_interfaz *interfaz = malloc(sizeof(t_interfaz));
    interfaz->nombre = string_duplicate(nombre_interfaz);
    interfaz->tipo = *tipo_interfaz;
    interfaz->conexion = conexion_io;
    interfaz->bloqueados = squeue_create();
    sem_init(&(interfaz->procesos_esperando), 0, 0); // Comienza en 0

    // Guardarlo en el diccionario.
    sdictionary_put(interfaces_conectadas, interfaz->nombre, interfaz);

    // Guardar el nombre en la lista
    slist_add(nombres_interfaces, string_duplicate(interfaz->nombre));

    list_destroy_and_destroy_elements(paquete_info_interfaz, free);

    return interfaz;
}

static void desregistrar_interfaz(t_interfaz *interfaz)
{
    // Eliminar la interfaz del diccionario
    sdictionary_remove(interfaces_conectadas, interfaz->nombre);

    // Eliminar de nombres_interfaces
    slist_lock(nombres_interfaces);
    t_list_iterator *it = list_iterator_create(nombres_interfaces->list);
    while (list_iterator_has_next(it)) {
        char *nombre = list_iterator_next(it);
        if (strcmp(nombre, interfaz->nombre) == 0) {
            list_iterator_remove(it);
            free(nombre);
            break;
        }
    }
    list_iterator_destroy(it);
    slist_unlock(nombres_interfaces);

    // Liberar t_interfaz
    free(interfaz->nombre);
    sem_destroy(&(interfaz->procesos_esperando));
    squeue_destroy(interfaz->bloqueados);
    free(interfaz);
}

static int enviar_operacion(t_bloqueado_io *pb, int conexion_io)
{
    switch (pb->opcode) {
    case GEN_SLEEP:
        return enviar_gen_sleep(pb, conexion_io);
    case STDIN_READ:
        return enviar_stdin_read(pb, conexion_io);
    case STDOUT_WRITE:
        return enviar_stdout_write(pb, conexion_io);
    case FS_CREATE:
        return enviar_fs_create(pb, conexion_io);
    case FS_DELETE:
        return enviar_fs_delete(pb, conexion_io);
    case FS_TRUNCATE:
        return enviar_fs_truncate(pb, conexion_io);
    case FS_WRITE:
        return enviar_fs_write(pb, conexion_io);
    case FS_READ:
        return enviar_fs_read(pb, conexion_io);
    }
    abort(); // Unreachable
}

static int enviar_gen_sleep(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = GEN_SLEEP;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));
    uint32_t *unidades_trabajo = list_get(pb->operacion, 2); // Es el 3er elemento, el 1er parametro.
    agregar_a_paquete(paquete, unidades_trabajo, sizeof(uint32_t));

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_stdin_read(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = STDIN_READ;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));
    size_t *tamanio_total = list_get(pb->operacion, 2); // Es el 3er elemento, el 1er parametro.
    agregar_a_paquete(paquete, tamanio_total, sizeof(size_t));

    t_list_iterator *it = list_iterator_create(pb->operacion);
    list_iterator_next(it); // Saltear nombre interfaz
    list_iterator_next(it); // Saltear operacion
    list_iterator_next(it); // Saltear tamanio total

    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);
        agregar_a_paquete(paquete, bloque, sizeof(t_bloque));
    }

    list_iterator_destroy(it);

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_stdout_write(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = STDOUT_WRITE;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));
    size_t *tamanio_total = list_get(pb->operacion, 2); // Es el 3er elemento, el 1er parametro.
    agregar_a_paquete(paquete, tamanio_total, sizeof(size_t));

    t_list_iterator *it = list_iterator_create(pb->operacion);
    list_iterator_next(it); // Saltear nombre interfaz
    list_iterator_next(it); // Saltear operacion
    list_iterator_next(it); // Saltear tamanio total

    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);
        agregar_a_paquete(paquete, bloque, sizeof(t_bloque));
    }

    list_iterator_destroy(it);

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_fs_create(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = FS_CREATE;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));

    char *nombre_archivo = list_get(pb->operacion, 2);
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_fs_delete(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = FS_DELETE;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));

    char *nombre_archivo = list_get(pb->operacion, 2);
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_fs_truncate(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = FS_TRUNCATE;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));

    char *nombre_archivo = list_get(pb->operacion, 2);
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

    size_t *tamanio = list_get(pb->operacion, 3);
    agregar_a_paquete(paquete, tamanio, sizeof(size_t));

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_fs_write(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = FS_WRITE;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));

    char *nombre_archivo = list_get(pb->operacion, 2);
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

    size_t *dir_archivo_inicio = list_get(pb->operacion, 3);
    agregar_a_paquete(paquete, dir_archivo_inicio, sizeof(size_t));

    size_t *tamanio = list_get(pb->operacion, 4);
    agregar_a_paquete(paquete, tamanio, sizeof(size_t));

    t_list_iterator *it = list_iterator_create(pb->operacion);
    list_iterator_next(it); // Saltear nombre interfaz
    list_iterator_next(it); // Saltear operacion
    list_iterator_next(it); // Saltear nombre archivo
    list_iterator_next(it); // Saltear dir inicio
    list_iterator_next(it); // Saltear tamanio

    // Agregar bloques
    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);
        agregar_a_paquete(paquete, bloque, sizeof(t_bloque));
    }
    list_iterator_destroy(it);

    int bytes = enviar_paquete(paquete, conexion_io);
    eliminar_paquete(paquete);

    return bytes;
}

static int enviar_fs_read(t_bloqueado_io *pb, int conexion_io)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pb->pcb->pid), sizeof(uint32_t));
    t_operacion_io op = FS_READ;
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));

    char *nombre_archivo = list_get(pb->operacion, 2);
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);

    size_t *dir_archivo_inicio = list_get(pb->operacion, 3);
    agregar_a_paquete(paquete, dir_archivo_inicio, sizeof(size_t));

    size_t *tamanio = list_get(pb->operacion, 4);
    agregar_a_paquete(paquete, tamanio, sizeof(size_t));

    t_list_iterator *it = list_iterator_create(pb->operacion);
    list_iterator_next(it); // Saltear nombre interfaz
    list_iterator_next(it); // Saltear operacion
    list_iterator_next(it); // Saltear nombre archivo
    list_iterator_next(it); // Saltear dir inicio
    list_iterator_next(it); // Saltear tamanio

    // Agregar bloques
    while (list_iterator_has_next(it)) {
        t_bloque *bloque = list_iterator_next(it);
        agregar_a_paquete(paquete, bloque, sizeof(t_bloque));
    }
    list_iterator_destroy(it);

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
