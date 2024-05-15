#ifndef SLIST_H_
#define SLIST_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

// Debe estar definido
extern t_log *debug_logger;

// Safe List, incluye un mutex
typedef struct {
    t_list *list;
    pthread_mutex_t *mutex;
} t_slist;

t_slist *slist_create();
void slist_destroy(t_slist *);
int slist_add(t_slist *, void *element);
void *slist_get(t_slist *, int index);
void *slist_remove_by_condition(t_slist *, bool(*condition)(void*));

#endif // SLIST_H_
