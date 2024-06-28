#ifndef BLOQUE_H
#define BLOQUE_H

#include <assert.h>
#include <commons/collections/list.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/mensajes.h>
#include <utils/sockets.h>

// Debe estar definido
extern t_log *debug_logger;

typedef struct {
    size_t base;    // Dirección Fisica donde inicia.
    size_t tamanio; // Bytes hasta el final.
} t_bloque;

// Envia OPCODE_LECTURA_ESPACIO_USUARIO y los contenidos del paquete para
// obtener un segmento de memoria.
void *leer_bloque_de_memoria(uint32_t pid, t_bloque bloque, int conexion_memoria);

// Envia OPCODE_ESCRITURA_ESPACIO_USUARIO, los contenidos del paquete y data para
// escribir a un segmento de memoria.
void escribir_bloque_a_memoria(uint32_t pid, t_bloque bloque, void *data, int conexion_memoria);

#endif // BLOQUE_H
