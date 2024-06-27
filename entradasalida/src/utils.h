#ifndef UTILS_ENTRADASALIDA_H_
#define UTILS_ENTRADASALIDA_H_

#include <commons/bitarray.h>
#include <assert.h>

// bitarray_clear_bit para un rango, fin inclusivo
void bitarray_clean_range(t_bitarray *bitmap, off_t inicio, off_t fin);

// bitarray_set_bit para un rango, fin inclusivo
void bitarray_set_range(t_bitarray *bitmap, off_t inicio, off_t fin);

#endif // UTILS_ENTRADASALIDA_H_
