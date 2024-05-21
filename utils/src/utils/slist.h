#ifndef SLIST_H_
#define SLIST_H_

#include <assert.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>

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
void *slist_remove_by_condition(t_slist *, bool (*condition)(void *));

void slist_lock(t_slist *);
void slist_unlock(t_slist *);

#endif // SLIST_H_
