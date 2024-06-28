#include "utils.h"

char *config_get_string_or_exit(t_config *config, char *key)
{
    if (!config_has_property(config, key)) {
        log_error(debug_logger, "La key %s no existe en el archivo de config", key);
        exit(1);
    }
    return config_get_string_value(config, key);
}

int config_get_int_or_exit(t_config *config, char *key)
{
    if (!config_has_property(config, key)) {
        log_error(debug_logger, "La key %s no existe en el archivo de config", key);
        exit(1);
    }
    return config_get_int_value(config, key);
}

int ceil_div(int dividend, int divisor)
{
    assert(divisor != 0);
    return (dividend % divisor) == 0 ? dividend / divisor : (dividend / divisor) + 1;
}

// Innecesario, pero ayuda a ser mas explicito al querer redondear hacia abajo.
int floor_div(int dividend, int divisor)
{
    assert(divisor != 0);
    return dividend / divisor;
}

char *print_hex(void *ptr, size_t size)
{
    char *str = string_new();

    string_append(&str, "0x");
    unsigned char *byte_ptr = (unsigned char *)ptr;
    for (size_t i = 0; i < size; i++) {
        char *bytes = string_from_format("%02x", byte_ptr[i]);
        string_append(&str, bytes);
        free(bytes);
    }

    return str;
}

size_t smin(size_t a, size_t b)
{
    return (a < b) ? a : b;
}
