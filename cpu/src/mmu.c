#include "main.h"

// Estructuras locales
typedef struct {
    uint32_t pid;
    uint32_t num_pag;
    uint32_t num_marco;
    uint64_t ultimo_acceso; // Nro. de instruccion
} t_tlb_entry;

size_t obtener_direccion_fisica(uint32_t pid, size_t dir_logica, int conexion_memoria)
{

}
