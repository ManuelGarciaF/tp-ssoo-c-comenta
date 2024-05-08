#include "sdictionary.h"

t_sdictionary *sdictionary_create()
{
    t_sdictionary *sd = malloc(sizeof(t_sdictionary));
    if (sd == NULL) {
        log_error(debug_logger, "Error al alojar memoria para sdictionary");
        return NULL;
    }
    sd->dict = dictionary_create();

    sd->mutex = malloc(sizeof(pthread_mutex_t));
    if (sd->mutex == NULL) {
        log_error(debug_logger, "Error al alojar memoria para mutex");
        free(sd);
        return NULL;
    }
    pthread_mutex_init(sd->mutex, NULL);

    return sd;
}

void sdictionary_destroy(t_sdictionary *sd)
{
    dictionary_destroy(sd->dict);
    pthread_mutex_destroy(sd->mutex);
    free(sd);
}

void *sdictionary_get(t_sdictionary *sd, char *key)
{
    pthread_mutex_lock(sd->mutex);
    void *elem = dictionary_get(sd->dict, key);
    pthread_mutex_unlock(sd->mutex);

    return elem;
}

void *sdictionary_remove(t_sdictionary *sd, char *key)
{
    pthread_mutex_lock(sd->mutex);
    void *elem = dictionary_remove(sd->dict, key);
    pthread_mutex_unlock(sd->mutex);

    return elem;
}

void sdictionary_put(t_sdictionary *sd, char *key, void *element)
{
    pthread_mutex_lock(sd->mutex);
    dictionary_put(sd->dict, key, element);
    pthread_mutex_unlock(sd->mutex);
}
