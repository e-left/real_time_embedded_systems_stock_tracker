#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define QUEUESIZE 20

typedef struct {
    double value;
    double volume;
    char *symbol;
    long timestamp;
    long received_timestamp;
} queue_value;

typedef struct {
    long head, tail;
    int full, empty;
    queue_value data[QUEUESIZE];
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, queue_value in);
void queueDel(queue *q, queue_value *out);

#endif