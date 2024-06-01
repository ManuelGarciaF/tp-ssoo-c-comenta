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
