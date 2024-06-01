#ifndef UTILS_H_
#define UTILS_H_

#include <assert.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>

// Debe estar definido
extern t_log *debug_logger;

/* Devuelve el valor de config, o loggea el error y hace exit(1) si no existe  */
char *config_get_string_or_exit(t_config *config, char *key);
int config_get_int_or_exit(t_config *config, char *key);

// Redondea hacia arriba una division de ints.
int ceil_div(int dividend, int divisor);

#endif // UTILS_H_
