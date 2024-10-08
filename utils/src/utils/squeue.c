#include "squeue.h"

t_squeue *squeue_create(void)
{
    t_squeue *sq = malloc(sizeof(t_squeue));
    if (sq == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    sq->queue = queue_create();

    sq->mutex = malloc(sizeof(pthread_mutex_t));
    if (sq->mutex == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }
    pthread_mutex_init(sq->mutex, NULL);

    return sq;
}

void squeue_destroy(t_squeue *sq)
{
    queue_destroy(sq->queue);
    pthread_mutex_destroy(sq->mutex);
    free(sq);
}

void squeue_destroy_and_destroy_elements(t_squeue *sq, void (*element_destroyer)(void *))
{
    queue_destroy_and_destroy_elements(sq->queue, element_destroyer);
    pthread_mutex_destroy(sq->mutex);
    free(sq);
}

void squeue_push(t_squeue *sq, void *element)
{
    pthread_mutex_lock(sq->mutex);
    queue_push(sq->queue, element);
    pthread_mutex_unlock(sq->mutex);
}

void *squeue_pop(t_squeue *sq)
{
    pthread_mutex_lock(sq->mutex);
    void *elem = queue_pop(sq->queue);
    pthread_mutex_unlock(sq->mutex);

    return elem;
}

void *squeue_peek(t_squeue *sq)
{
    pthread_mutex_lock(sq->mutex);
    void *elem = queue_peek(sq->queue);
    pthread_mutex_unlock(sq->mutex);

    return elem;
}

bool squeue_is_empty(t_squeue *sq)
{
    return queue_is_empty(sq->queue);
}

int squeue_size(t_squeue *sq)
{
    return queue_size(sq->queue);
}

void squeue_lock(t_squeue *sq)
{
    pthread_mutex_lock(sq->mutex);
}
void squeue_unlock(t_squeue *sq)
{
    pthread_mutex_unlock(sq->mutex);
}
