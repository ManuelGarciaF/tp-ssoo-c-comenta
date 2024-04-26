#ifndef SQUEUE_H_
#define SQUEUE_H_

#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdlib.h>
#include <commons/log.h>

// Debe estar definido
extern t_log *debug_logger;

// Safe Queue, incluye un mutex
typedef struct {
    t_queue *queue;
    pthread_mutex_t *mutex;
} t_squeue;

t_squeue *squeue_create();
void squeue_destroy(t_squeue *);
void squeue_destroy_and_destroy_elements(t_squeue *,
                                         void (*element_destroyer)(void *));
void squeue_push(t_squeue *, void *element);
void *squeue_pop(t_squeue *);

#endif // SQUEUE_H_
