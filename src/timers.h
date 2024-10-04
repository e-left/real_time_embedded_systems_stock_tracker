#ifndef TIMERS_H
#define TIMERS_H

#include <pthread.h>

typedef struct {
    long delay_s;
    pthread_cond_t* tv;
    long* reference_timestamp;
    pthread_mutex_t* t_mutex;
    int* ready;
} timer_args;

timer_args* initialize_timer_args(long delay_s, pthread_cond_t* tv, long* reference_timestamp, pthread_mutex_t* t_mutex, int* ready);
void start_timer(timer_args* t_args);

#endif