#ifndef SDICTIONARY_H_
#define SDICTIONARY_H_

#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

// Debe estar definido
extern t_log *debug_logger;

// Safe dictionary, incluye un mutex
typedef struct {
    t_dictionary *dict;
    pthread_mutex_t *mutex;
} t_sdictionary;

// Definiciones Funciones
void *sdictionary_get(t_sdictionary *, char *key);
void *sdictionary_remove(t_sdictionary *, char *key);
void sdictionary_put(t_sdictionary *, char *key, void *element);
t_sdictionary *sdictionary_create();
void sdictionary_destroy(t_sdictionary *);
void sdictionary_destroy_and_destroy_elements(t_sdictionary *, void (*element_destroyer)(void *));
bool sdictionary_has_key(t_sdictionary *, char *key);

#endif // SDICTIONARY_H_
