#include "main.h"

static uint32_t ultimo_pid = 0;

// Definiciones locales
static void ejecutar_comando(char const *linea);
static void iniciar_proceso(char const *path);
static void finalizar_proceso(char const *pid);
static void actualizar_grado_multiprogramacion(int nuevo);
static void ejecutar_script(char const *path);
static void imprimir_estado_procesos(void);

#define LIMITE_LINEA_COMANDO 256

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

void ejecutar_comando(char const *linea)
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
        pausar_planificacion();

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

static void iniciar_proceso(char const *path)
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

static void finalizar_proceso(char const *pid)
{
    // TODO
    assert(false && "No implementado");
}

static void actualizar_grado_multiprogramacion(int nuevo)
{
    int diferencia = nuevo - grado_multiprogramacion;

    grado_multiprogramacion = nuevo;

    // Hay que agregar espacio
    if (diferencia > 0) {
        for (int i = 0; i < diferencia; i++) {
            sem_post(&sem_multiprogramacion);
        }
    } else if (diferencia < 0) { // Hay que quitar espacio (diferencia es negativo)
        int procesos_extra = -diferencia;
        procesos_extra_multiprogramacion += procesos_extra;
    }
}

static void ejecutar_script(char const *path)
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
    imprimir_bloqueados_por_recursos(colas_blocked_recursos);
    printf(" └Bloqueados por I/O:\n");
    imprimir_bloqueados_por_io(interfaces_conectadas);
}
