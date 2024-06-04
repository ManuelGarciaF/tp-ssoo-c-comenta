#include "main.h"

// Estructuras locales
typedef struct {
    uint32_t pid;
    uint32_t num_pagina;
    uint32_t num_marco;
    uint64_t ultimo_acceso; // Nro. de instruccion
} t_tlb_entry;

// Variables locales
static t_queue *tlb_entries; // Contiene t_tlb_entry

// Funciones locales
static bool tlb_get(uint32_t pid, uint32_t num_pagina, uint32_t *num_marco);
static uint32_t buscar_en_tabla_de_paginas(uint32_t pid, size_t num_pagina);
static void tlb_save(uint32_t pid, uint32_t num_pagina, uint32_t num_marco);
static void fifo_remover_victima(t_queue *entries);
static void lru_remover_victima(t_queue *entries);

void inicializar_mmu(void)
{
    tlb_entries = queue_create();
}

size_t obtener_direccion_fisica(uint32_t pid, size_t dir_logica)
{
    uint32_t num_pagina = dir_logica / tam_pagina; // Division entera, se redondea hacia abajo
    uint32_t offset = dir_logica % tam_pagina;

    uint32_t num_marco;

    // Si no esta en la tlb, buscarlo en memoria y guardarlo en la tlb.
    if (tlb_get(pid, num_pagina, &num_marco) == true) {
        log_info(cpu_logger, "PID: %u - TLB HIT - Pagina: %u", pid, num_pagina);
    } else {
        log_info(cpu_logger, "PID: %u - TLB MISS - Pagina: %u", pid, num_pagina);
        num_marco = buscar_en_tabla_de_paginas(pid, num_pagina);
        log_info(cpu_logger, "PID: %u OBTENER MARCO Página: %u Marco: %u", pid, num_pagina, num_marco);
        tlb_save(pid, num_pagina, num_marco);
    }

    // Calcular la direccion fisica
    return (num_marco * tam_pagina) + offset;
}

bool entra_en_pagina(size_t dir_logica, size_t tamanio)
{
    uint32_t offset = dir_logica % tam_pagina;
    return offset + tamanio <= tam_pagina; // TODO checkear este calculo.
}

size_t tam_restante_pag(size_t dir_logica)
{
    uint32_t offset = dir_logica % tam_pagina;
    return tam_pagina - offset; // TODO checkear este calculo.
}

void *leer_espacio_usuario(uint32_t pid, size_t dir_logica, size_t tamanio)
{
    // Nunca deberiamos leer afuera de 1 pagina
    assert(entra_en_pagina(dir_logica, tamanio));

    size_t dir_fisica = obtener_direccion_fisica(pid, dir_logica);

    // Enviar opcode
    enviar_int(OPCODE_LECTURA_ESPACIO_USUARIO, conexion_memoria);

    // Enviar pid, inicio y tamanio
    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, &pid, sizeof(uint32_t));
    agregar_a_paquete(p, &dir_fisica, sizeof(size_t));
    agregar_a_paquete(p, &tamanio, sizeof(size_t));
    enviar_paquete(p, conexion_memoria);
    eliminar_paquete(p);

    t_list *p_respuesta = recibir_paquete(conexion_memoria);
    void *respuesta = list_get(p_respuesta, 0);
    list_destroy(p_respuesta);

    char *hexstring = print_hex(respuesta, tamanio);
    log_info(debug_logger, "PID: %u Acción: LEER -  Dirección Física: %zu - Valor: %s", pid, dir_fisica, hexstring);
    free(hexstring);

    return respuesta;
}

void escribir_espacio_usuario(uint32_t pid, size_t dir_logica, const void *datos, size_t tamanio)
{
    // Nunca deberiamos escribir afuera de 1 pagina
    assert(entra_en_pagina(dir_logica, tamanio));

    size_t dir_fisica = obtener_direccion_fisica(pid, dir_logica);

    // Enviar opcode
    enviar_int(OPCODE_ESCRITURA_ESPACIO_USUARIO, conexion_memoria);

    // Enviar pid, inicio y tamanio
    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, &pid, sizeof(uint32_t));
    agregar_a_paquete(p, &dir_fisica, sizeof(size_t));
    agregar_a_paquete(p, &tamanio, sizeof(size_t));
    agregar_a_paquete(p, (void *)datos, tamanio);
    enviar_paquete(p, conexion_memoria);
    eliminar_paquete(p);

    // Esperar que responda memoria
    if (recibir_int(conexion_memoria) != MENSAJE_FIN_ESCRITURA) {
        log_error(debug_logger, "La memoria no devolvio MENSAJE_FIN_ESCRITURA");
        abort();
    }

    char *hexstring = print_hex((void *)datos, tamanio);
    log_info(debug_logger, "PID: %u Acción: ESCRIBIR -  Dirección Física: %zu - Valor: %s", pid, dir_fisica, hexstring);
    free(hexstring);
}

/*
** Funciones privadas
*/

// Retorna true si fue encontrado, si lo encuentra guarda el numero de marco en num_marco.
static bool tlb_get(uint32_t pid, uint32_t num_pagina, uint32_t *num_marco)
{
    bool encontrado = false;

    t_list_iterator *it = list_iterator_create(tlb_entries->elements);
    while (list_iterator_has_next(it)) {
        t_tlb_entry *entry = list_iterator_next(it);
        if (entry->pid == pid && entry->num_pagina == num_pagina) {
            encontrado = true;

            *num_marco = entry->num_marco;

            // Actualizar el ultimo acceso
            entry->ultimo_acceso = num_instruccion_actual;

            break;
        }
    }
    list_iterator_destroy(it);

    return encontrado;
}

// Solicita a memoria el numero de marco correspondiente
static uint32_t buscar_en_tabla_de_paginas(uint32_t pid, size_t num_pagina)
{
    // Buscar la pagina en memoria.
    enviar_int(OPCODE_ACCESO_TABLA_PAGINAS, conexion_memoria);
    // Enviar pid y num_pagina.
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &num_pagina, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    return recibir_int(conexion_memoria);
}

static void tlb_save(uint32_t pid, uint32_t num_pagina, uint32_t num_marco)
{
    if (cantidad_entradas_tlb == 0) {
        return;
    }

    // Crear la entry nueva.
    t_tlb_entry *entry = malloc(sizeof(t_tlb_entry));
    assert(entry != NULL);
    entry->pid = pid;
    entry->num_pagina = num_pagina;
    entry->num_marco = num_marco;
    entry->ultimo_acceso = num_instruccion_actual;

    // Si la tlb ya esta llena
    if (queue_size(tlb_entries) >= cantidad_entradas_tlb) {
        // Sacar una entrada segun el algoritmo
        switch (algoritmo_tlb) {
        case FIFO:
            fifo_remover_victima(tlb_entries);
            break;
        case LRU:
            lru_remover_victima(tlb_entries);
            break;
        default:
            abort(); // Unreachable
        }
    }

    // Deberia haber menos entradas que el limite antes de agregar una nueva
    assert(queue_size(tlb_entries) < cantidad_entradas_tlb);

    queue_push(tlb_entries, entry);
}

static void fifo_remover_victima(t_queue *entries)
{
    // Removemos la entrada mas antigua.
    t_tlb_entry *victima = queue_pop(entries);
    free(victima);
}

static void lru_remover_victima(t_queue *entries)
{
    // Buscamos el elemento que no se accede hace mas tiempo.
    t_list_iterator *it = list_iterator_create(entries->elements);

    // Obtener el primer elemento
    assert(list_iterator_has_next(it)); // Nunca deberiamos llamar esta funcion cuando la tlb tiene 0 entradas
    t_tlb_entry *victima = list_iterator_next(it);
    int indice_victima = list_iterator_index(it); // Deberia ser 0

    while (list_iterator_has_next(it)) {
        t_tlb_entry *actual = list_iterator_next(it);
        // Si el ultimo acceso del actual es mas viejo que la victima actual
        if (actual->ultimo_acceso < victima->ultimo_acceso) {
            indice_victima = list_iterator_index(it);
            victima = actual;
        }
    }
    list_iterator_destroy(it);

    // Sacar la victima de la lista y liberarla
    list_remove(entries->elements, indice_victima);
    free(victima);
}
