#include "utils.h"

void bitarray_clean_range(t_bitarray *bitmap, off_t inicio, off_t fin)
{
    assert(inicio <= fin);
    for (off_t i = inicio; i <= fin; i++) {
        bitarray_clean_bit(bitmap, i);
    }
}

void bitarray_set_range(t_bitarray *bitmap, off_t inicio, off_t fin)
{
    assert(inicio <= fin);
    for (off_t i = inicio; i <= fin; i++) {
        bitarray_set_bit(bitmap, i);
    }
}
