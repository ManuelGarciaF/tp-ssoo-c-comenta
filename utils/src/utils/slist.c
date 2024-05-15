#include "slist.h"

t_slist *slist_create()
{
    t_slist *sl = malloc(sizeof(t_slist));
    assert(sl != NULL);
    sl->list = list_create();

    sl->mutex = malloc(sizeof(pthread_mutex_t));
    assert(sl->mutex != NULL);
    pthread_mutex_init(sl->mutex, NULL);

    return sl;
}

void slist_destroy(t_slist *sl)
{
    list_destroy(sl->list);
    pthread_mutex_destroy(sl->mutex);
    free(sl);
}

int slist_add(t_slist *sl, void *element)
{
    pthread_mutex_lock(sl->mutex);
    int pos = list_add(sl->list, element);
    pthread_mutex_unlock(sl->mutex);

    return pos;
}

void *slist_get(t_slist *sl, int index)
{
    pthread_mutex_lock(sl->mutex);
    void *elem = list_get(sl->list, index);
    pthread_mutex_unlock(sl->mutex);

    return elem;
}

void *slist_remove_by_condition(t_slist *sl, bool (*condition)(void *))
{
    pthread_mutex_lock(sl->mutex);
    void *elem = list_remove_by_condition(sl->list, condition);
    pthread_mutex_unlock(sl->mutex);

    return elem;
}
