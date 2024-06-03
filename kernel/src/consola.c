#include "main.h"

static uint32_t ultimo_pid = 0;

// Definiciones locales
static void ejecutar_comando(const char *linea);

static void iniciar_proceso(const char *path);
static void finalizar_proceso(const char *pid_str);
static void actualizar_grado_multiprogramacion(int nuevo);
static void ejecutar_script(const char *path);
static void imprimir_estado_procesos(void);

static bool buscar_y_eliminar_en_new(uint32_t pid);
static bool buscar_y_eliminar_en_ready(uint32_t pid);
static bool buscar_y_eliminar_en_blocked_recursos(uint32_t pid);
static bool buscar_y_eliminar_en_blocked_interfaces(uint32_t pid);

static void imprimir_bloqueados_por_recursos(void);
static void imprimir_bloqueados_por_io(void);
static void imprimir_bloqueados_recurso(char *nombre_recurso, void *puntero_cola);
static void imprimir_bloqueados_interfaz(char *nombre_interfaz, void *puntero_interfaz);

#define LIMITE_LINEA_COMANDO 256

// Funcion llamada desde main
void correr_consola(void)
{
    char *input;
    while (true) {
        // Esperar unos milisegundos para que no se desordene por los logs
        // producidos por los planificadores
        usleep(50 * 1000); // TODO ver si esto es ilegal
        input = readline("kernel> ");
        if (input) {
            add_history(input);
        }
        if (!input) {
            break;
        }

        ejecutar_comando(input);

        free(input);
    }
}

void ejecutar_comando(const char *linea)
{
    // Dividir input en comando y parametro
    char **comando = string_n_split((char *)linea, 2, " ");

    if (!strcmp(comando[0], "EJECUTAR_SCRIPT")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        ejecutar_script(comando[1]);

    } else if (!strcmp(comando[0], "INICIAR_PROCESO")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        iniciar_proceso(comando[1]);

    } else if (!strcmp(comando[0], "FINALIZAR_PROCESO")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        finalizar_proceso(comando[1]);

    } else if (!strcmp(comando[0], "DETENER_PLANIFICACION")) {
        if (!planificacion_pausada) {
            pausar_planificacion();
        }

    } else if (!strcmp(comando[0], "INICIAR_PLANIFICACION")) {
        if (planificacion_pausada) {
            reanudar_planificacion();
        }

    } else if (!strcmp(comando[0], "MULTIPROGRAMACION")) {
        if (comando[1] == NULL) {
            printf("error: El comando necesita un parametro.\n");
            return;
        }
        actualizar_grado_multiprogramacion(atoi(comando[1]));

    } else if (!strcmp(comando[0], "PROCESO_ESTADO")) {
        imprimir_estado_procesos();
    } else {
        printf("Comando no reconocido\n");
    }

    string_array_destroy(comando);
}

static void iniciar_proceso(const char *path)
{
    // Guardar tanto el pid como el path, para enviar a memoria
    t_proceso_nuevo *proceso_nuevo = malloc(sizeof(t_proceso_nuevo));
    assert(proceso_nuevo != NULL);

    proceso_nuevo->pid = ultimo_pid++;
    // Hay que duplicarlo para poder liberar el comando despues
    proceso_nuevo->path = string_duplicate((char *)path);

    // Agregarlo a la lista de new
    squeue_push(cola_new, proceso_nuevo);
    sem_post(&sem_elementos_en_new); // Avisamos que hay 1 elemento mas en NEW

    log_info(kernel_logger, "Se crea el proceso %d en NEW", proceso_nuevo->pid);
    // Ahora depende del planificador a largo plazo.
}

static void finalizar_proceso(const char *pid_str)
{
    uint32_t pid = atoi(pid_str);
    printf("Buscando proceso con pid: %u\n", pid);

    // Evitar que los procesos cambien de estado.
    bool reanudar = false;
    if (!planificacion_pausada) { // Solo pausar cuando no estaba pausado.
        reanudar = true;
        pausar_planificacion();
    }

    // Ver si se encuentra en ejecucion
    if (pid_en_ejecucion != -1 && pid_en_ejecucion == (int)pid) {
        log_info(debug_logger, "Se encontro el proceso en EXEC");
        // Desalojar el proceso y eliminarlo cuando vuelva
        t_paquete *paquete = crear_paquete();
        agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
        t_motivo_desalojo motivo = INTERRUMPIDO_POR_USUARIO;
        agregar_a_paquete(paquete, &motivo, sizeof(t_motivo_desalojo));
        enviar_paquete(paquete, conexion_cpu_interrupt);
        eliminar_paquete(paquete);
    } else {
        // Hay que buscarlo en todas las otras colas
        bool encontrado = buscar_y_eliminar_en_new(pid);
        if (!encontrado) { // No estaba en new, buscar en ready
            encontrado = buscar_y_eliminar_en_ready(pid);
        }
        if (!encontrado) { // No estaba en ready, buscar en bloqueados por recurso
            encontrado = buscar_y_eliminar_en_blocked_recursos(pid);
        }
        if (!encontrado) { // No estaba bloqueado por recuros, buscar en bloqueados por io
            encontrado = buscar_y_eliminar_en_blocked_interfaces(pid);
        }

        if (encontrado) { // No se encontro el proceso en ninguna cola
            log_info(kernel_logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", pid);
            log_info(kernel_logger, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", pid);
        } else {
            printf("error: No se encontro el proceso con pid: %u\n", pid);
        }
    }

    // Volver a habilitar la planificacion.
    if (reanudar) { // Solo reanudar si fue pausada por este comando
        reanudar_planificacion();
    }
}

static void actualizar_grado_multiprogramacion(int nuevo)
{
    int diferencia = nuevo - grado_multiprogramacion;

    grado_multiprogramacion = nuevo;

    // Hay que agregar espacio, solamente incrementamos el semaforo
    if (diferencia > 0) {
        for (int i = 0; i < diferencia; i++) {
            sem_post(&sem_multiprogramacion);
        }
    } else if (diferencia < 0) { // Hay que quitar espacio (diferencia es negativo)
        int procesos_extra = -diferencia;
        procesos_extra_multiprogramacion += procesos_extra;
    }
}

static void ejecutar_script(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("error: No se pudo abrir el archivo de script.\n");
        return;
    }

    // Alojar espacio en el stack
    char linea[LIMITE_LINEA_COMANDO] = {0};

    char *ret = fgets(linea, LIMITE_LINEA_COMANDO, fp); // Leer la primera linea
    while (ret != NULL) {
        // Quitar el \n final leido por fgets
        if (linea[strlen(linea) - 1] == '\n') {
            linea[strlen(linea) - 1] = '\0';
        }

        printf("<SCRIPT> Ejecutando: %s\n", linea);
        ejecutar_comando(linea);

        ret = fgets(linea, LIMITE_LINEA_COMANDO, fp);
    }
}

static void imprimir_estado_procesos(void)
{
    // Elementos en NEW:
    char *pids_new = obtener_lista_pids_proceso_nuevo(cola_new);
    printf("NEW: [%s]\n", pids_new);
    free(pids_new);

    // Elementos en READY:
    char *pids_ready = obtener_lista_pids_pcb(cola_ready);
    printf("READY: [%s]\n", pids_ready);
    free(pids_ready);

    // Proceso en EXEC:
    if (pid_en_ejecucion >= 0) {
        printf("EXEC: [%d]\n", pid_en_ejecucion);
    } else { // Esta en -1 cuando no hay procesos ejecutando
        printf("EXEC: []\n");
    }

    // Procesos bloqueados:
    printf("BLOCKED:\n");
    printf(" ├Bloqueados por Recursos:\n");
    imprimir_bloqueados_por_recursos();
    printf(" └Bloqueados por I/O:\n");
    imprimir_bloqueados_por_io();

    char *pids_exit = obtener_lista_pids_exit(cola_exit);
    printf("EXIT: [%s]\n", pids_exit);
    free(pids_exit);
}

static bool buscar_y_eliminar_en_new(uint32_t pid)
{
    bool encontrado = false;
    squeue_lock(cola_new);
    t_list_iterator *it = list_iterator_create(cola_new->queue->elements);

    while (list_iterator_has_next(it) && !encontrado) {
        t_proceso_nuevo *pn = list_iterator_next(it);
        if (pn->pid == pid) {
            encontrado = true;
            log_info(debug_logger, "Se encontro el proceso en la cola de NEW");

            // Agregar el pid a exit
            agregar_a_exit(pn->pid);

            // Eliminar y liberar pn
            list_iterator_remove(it);
            free(pn->path);
            free(pn);

        }
    }

    list_iterator_destroy(it);
    squeue_unlock(cola_new);
    return encontrado;
}

static bool buscar_y_eliminar_en_ready(uint32_t pid)
{
    bool encontrado = false;
    squeue_lock(cola_ready);
    t_list_iterator *it = list_iterator_create(cola_ready->queue->elements);

    while (list_iterator_has_next(it) && !encontrado) {
        t_pcb *pcb = list_iterator_next(it);
        if (pcb->pid == pid) {
            encontrado = true;
            log_info(debug_logger, "Se encontro el proceso en la cola de READY");
            list_iterator_remove(it);
            eliminar_proceso(pcb);
        }
    }

    list_iterator_destroy(it);
    squeue_unlock(cola_ready);
    return encontrado;
}

static bool buscar_y_eliminar_en_blocked_recursos(uint32_t pid)
{
    bool encontrado = false;
    slist_lock(nombres_recursos);

    // Iterar sobre los recursos
    t_list_iterator *it_nombres = list_iterator_create(nombres_recursos->list);
    while (list_iterator_has_next(it_nombres) && !encontrado) {
        char *nombre_recurso = list_iterator_next(it_nombres);
        t_squeue *bloqueados = sdictionary_get(colas_blocked_recursos, nombre_recurso);

        // Buscar en los bloqueados de este recurso
        squeue_lock(bloqueados);
        t_list_iterator *it_bloqueados = list_iterator_create(bloqueados->queue->elements);

        while (list_iterator_has_next(it_bloqueados) && !encontrado) {
            t_pcb *pcb = list_iterator_next(it_bloqueados);
            if (pcb->pid == pid) {
                encontrado = true;
                log_info(debug_logger,
                         "Se encontro el proceso en la cola de bloqueados del recurso %s",
                         nombre_recurso);
                list_iterator_remove(it_bloqueados);
                eliminar_proceso(pcb);

                // Incrementar las instancias del recurso, ya que hay un proceso menos esperando
                int *cant_recurso = sdictionary_get(instancias_recursos, nombre_recurso);
                assert(*cant_recurso < 0);
                (*cant_recurso)++; // No hay que preocuparse de condiciones de carrera, ya que la
                                   // planificacion esta pausada.
            }
        }

        list_iterator_destroy(it_bloqueados);
        squeue_unlock(bloqueados);
    }

    list_iterator_destroy(it_nombres);
    slist_unlock(nombres_recursos);

    return encontrado;
}

static bool buscar_y_eliminar_en_blocked_interfaces(uint32_t pid)
{
    bool encontrado = false;
    slist_lock(nombres_interfaces);

    // Iterar sobre las interfaces
    t_list_iterator *it_nombres = list_iterator_create(nombres_interfaces->list);
    while (list_iterator_has_next(it_nombres) && !encontrado) {
        char *nombre_interfaz = list_iterator_next(it_nombres);
        t_interfaz *interfaz = sdictionary_get(interfaces_conectadas, nombre_interfaz);
        assert(interfaz != NULL);

        // Buscar en los bloqueados de esta interfaz
        squeue_lock(interfaz->bloqueados);
        t_list_iterator *it_bloqueados = list_iterator_create(interfaz->bloqueados->queue->elements);

        while (list_iterator_has_next(it_bloqueados) && !encontrado) {
            t_bloqueado_io *pb = list_iterator_next(it_bloqueados);
            if (pb->pcb->pid == pid) {
                encontrado = true;
                log_info(debug_logger,
                         "Se encontro el proceso en la cola de bloqueados de la interfaz %s",
                         interfaz->nombre);
                list_iterator_remove(it_bloqueados);
                eliminar_proceso(pb->pcb);
                // Liberar el resto de bi
                list_destroy_and_destroy_elements(pb->operacion, free);
                free(pb);
            }
        }

        list_iterator_destroy(it_bloqueados);
        squeue_unlock(interfaz->bloqueados);
    }

    list_iterator_destroy(it_nombres);
    slist_unlock(nombres_interfaces);

    return encontrado;
}

static void imprimir_bloqueados_por_recursos(void)
{
    sdictionary_iterator(colas_blocked_recursos, imprimir_bloqueados_recurso);
}

static void imprimir_bloqueados_por_io(void)
{
    sdictionary_iterator(interfaces_conectadas, imprimir_bloqueados_interfaz);
}

static void imprimir_bloqueados_recurso(char *nombre_recurso, void *puntero_cola)
{
    t_squeue *bloqueados = puntero_cola;

    printf(" │ └%s: ", nombre_recurso);

    squeue_lock(bloqueados);

    t_list_iterator *it = list_iterator_create(bloqueados->queue->elements);
    if (list_iterator_has_next(it)) { // La lista tiene al menos 1 elemento
        t_pcb *primer_pcb = list_iterator_next(it);
        printf("[%d", primer_pcb->pid);
        while (list_iterator_has_next(it)) {
            t_pcb *pcb = list_iterator_next(it);
            printf(", %d", pcb->pid);
        }
        printf("]\n");
    } else { // La lista esta vacia
        printf("[]\n");
    }
    list_iterator_destroy(it);
    squeue_unlock(bloqueados);
}

static void imprimir_bloqueados_interfaz(char *nombre_interfaz, void *puntero_interfaz)
{
    t_interfaz *interfaz = puntero_interfaz;

    printf("   └%s: ", nombre_interfaz);

    squeue_lock(interfaz->bloqueados);

    t_list_iterator *it = list_iterator_create(interfaz->bloqueados->queue->elements);
    if (list_iterator_has_next(it)) { // La lista tiene al menos 1 elemento
        t_bloqueado_io *primer_bloqueado = list_iterator_next(it);
        printf("[%d", primer_bloqueado->pcb->pid);
        while (list_iterator_has_next(it)) {
            t_bloqueado_io *bloqueado = list_iterator_next(it);
            printf(", %d", bloqueado->pcb->pid);
        }
        printf("]\n");
    } else { // La lista esta vacia
        printf("[] \n");
    }
    list_iterator_destroy(it);
    squeue_unlock(interfaz->bloqueados);
}
