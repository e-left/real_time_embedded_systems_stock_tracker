#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "timers.h"

timer_args* initialize_timer_args(long delay_s, pthread_cond_t* tv, long* reference_timestamp, pthread_mutex_t* t_mutex, int* ready) {
    timer_args* t_args = (timer_args*) malloc(sizeof(timer_args));
    if(t_args == NULL) {
        printf("Error intializing timer arguments. Exiting...\n");
        exit(1);
    }

    t_args->tv = tv;
    t_args->delay_s = delay_s;
    t_args->reference_timestamp = reference_timestamp;
    t_args->t_mutex = t_mutex;
    t_args->ready = ready;

    return t_args;
}

void* timer_func(void* t_worker_args) {
    timer_args* t_args = (timer_args*) t_worker_args; 

    while(1) {
        usleep(t_args->delay_s * 1000 * 1000);
        
        struct timeval reference_timestamp;
        gettimeofday(&reference_timestamp, NULL);
        long current_reference_timestamp = reference_timestamp.tv_sec * 1000 + reference_timestamp.tv_usec / 1000;
        *(t_args->reference_timestamp) = current_reference_timestamp;
        
        pthread_mutex_lock(t_args->t_mutex);
        *(t_args->ready) = 1;
        pthread_mutex_unlock(t_args->t_mutex);
        pthread_cond_signal(t_args->tv);
    }
}

void start_timer(timer_args* t_args) {
    pthread_t timer_t;

    pthread_create(&timer_t, NULL, timer_func, (void *)t_args);
}
