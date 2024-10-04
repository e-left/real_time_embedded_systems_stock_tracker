#include <float.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "linkedlist.h"
#include "queue.h"
#include "timers.h"
#include "workers.h"

typedef struct {
    const char *symbol;
    Node *list;
} stored_value;

stored_value *values;
pthread_mutex_t values_mut;
int n;

void setup_workers(const char **provided_tickers, int provided_tickers_n) {
    pthread_mutex_init(&values_mut, NULL);
    values = (stored_value *)malloc(provided_tickers_n * sizeof(stored_value));
    if (values == NULL) {
        printf("Error initializing values. Exiting...\n");
    }

    n = provided_tickers_n;

    for (int i = 0; i < provided_tickers_n; i++) {
        values[i].symbol = provided_tickers[i];
        values[i].list = NULL;
    }
}

void clear_workers() {
    pthread_mutex_destroy(&values_mut);
    for (int i = 0; i < n; i++) {
        delete_list(values[i].list);
    }

    free(values);
}

logger_args *initialize_logger_args(queue *q, file_collection *fc_prices) {
    logger_args *l_args = (logger_args *)malloc(sizeof(logger_args));
    if (l_args == NULL) {
        printf("Error initializing logger args. Exiting...\n");
        exit(1);
    }

    l_args->q = q;
    l_args->fc_prices = fc_prices;

    return l_args;
}

void delete_logger_args(logger_args *l_args) {
    free(l_args);
}

moving_average_args *initialize_moving_average_args(long delay, file_collection *fc_ma) {
    moving_average_args *ma_args = (moving_average_args *)malloc(sizeof(moving_average_args));
    if (ma_args == NULL) {
        printf("Error initializing moving average args. Exiting...\n");
        exit(1);
    }

    ma_args->delay = delay;
    ma_args->fc_ma = fc_ma;

    return ma_args;
}

void delete_moving_average_args(moving_average_args *ma_args) {
    free(ma_args);
}

candlestick_args *initialize_candlestick_args(long delay, file_collection *fc_c) {
    candlestick_args *c_args = (candlestick_args *)malloc(sizeof(candlestick_args));
    if (c_args == NULL) {
        printf("Error initializing candlestick args. Exiting...\n");
        exit(1);
    }

    c_args->delay = delay;
    c_args->fc_c = fc_c;

    return c_args;
}

void delete_candlestick_args(candlestick_args *c_args) {
    free(c_args);
}

// function for the thread that will log all
void *logger_worker(void *args) {
    // unpack arguments, and queue
    logger_args *l_args = (logger_args *)args;
    queue *l_queue = l_args->q;

    // wait for values in the queue
    while (1) {
        // wait untill we get a value
        queue_value data_point;
        pthread_mutex_lock(l_queue->mut);
        // conditionally wait without locking the mutex
        while (l_queue->empty) {
            pthread_cond_wait(l_queue->notEmpty, l_queue->mut);
        }

        // extract value, unlock mutex, signal the queue to be not full
        queueDel(l_queue, &data_point);
        pthread_mutex_unlock(l_queue->mut);
        pthread_cond_signal(l_queue->notFull);

        // write the received value to the respective file
        for (int i = 0; i < n; i++) {
            if (strcmp(data_point.symbol, l_args->fc_prices->files[i].symbol) == 0) {

                // get timestamp here and log it as logging timestamp
                struct timeval logging_timestamp;
                gettimeofday(&logging_timestamp, NULL);
                long current_logging_timestamp = logging_timestamp.tv_sec * 1000 + logging_timestamp.tv_usec / 1000;

                fprintf(l_args->fc_prices->files[i].f, "%f, %f, %ld, %ld, %ld\n", data_point.value, data_point.volume, data_point.timestamp, data_point.received_timestamp, current_logging_timestamp);
            }
        }

        // add to the respective linked lists for better management
        pthread_mutex_lock(&values_mut);

        for (int i = 0; i < n; i++) {
            if (strcmp(data_point.symbol, values[i].symbol) == 0) {
                values[i].list = add_list_item(values[i].list, data_point.value, data_point.volume, data_point.timestamp);
            }
        }

        pthread_mutex_unlock(&values_mut);

        // cleanup
        free(data_point.symbol);
    }
    return (NULL);
}

// These two workers will start and wait for timer signals
// This will ensure accurate time keeping and logging
void *candlestick_worker(void *args) {
    candlestick_args *c_args = (candlestick_args *)args;
    // initialize and start a 15 minute timer
    pthread_cond_t tv;
    pthread_cond_init(&tv, NULL);

    pthread_mutex_t t_mutex;
    pthread_mutex_init(&t_mutex, NULL);

    int ready = 0;

    long current_reference_timestamp = 0;

    timer_args *t_args = initialize_timer_args(60, &tv, &current_reference_timestamp, &t_mutex, &ready);
    start_timer(t_args);

    printf("Candlestick timer started successfully\n");

    // main working task
    while (1) {

        pthread_mutex_lock(&t_mutex);
        while (!ready) {
            pthread_cond_wait(&tv, &t_mutex);
        }
        ready = 0;
        pthread_mutex_unlock(&t_mutex);

        // printf("[*] Logging candlestick\n");

        pthread_mutex_lock(&values_mut);
        for (int i = 0; i < n; i++) {
            if (values[i].list != NULL) {

                Node *ptr = get_most_recent_items(values[i].list, c_args->delay);

                if (ptr != NULL) {

                    double open = 0;
                    double close = 0;
                    double low = DBL_MAX;
                    double high = DBL_MIN;
                    double total_volume = 0;

                    open = ptr->value;

                    while (ptr != NULL) {
                        close = ptr->value;

                        if (ptr->value > high) {
                            high = ptr->value;
                        }

                        if (ptr->value < low) {
                            low = ptr->value;
                        }

                        total_volume += ptr->volume;

                        ptr = ptr->next;
                    }

                    // write the candlestick to the respective file
                    // get timestamp here and log it as logging timestamp
                    struct timeval logging_timestamp;
                    gettimeofday(&logging_timestamp, NULL);
                    long current_logging_timestamp = logging_timestamp.tv_sec * 1000 + logging_timestamp.tv_usec / 1000;

                    fprintf(c_args->fc_c->files[i].f, "%f, %f, %f, %f, %f, %ld, %ld\n", open, high, low, close, total_volume, current_reference_timestamp, current_logging_timestamp);
                }
            }
        }

        pthread_mutex_unlock(&values_mut);
    }

    return NULL;
}

void *moving_average_worker(void *args) {
    moving_average_args *ma_args = (moving_average_args *)args;

    // initialize and start a 15 minute timer
    pthread_cond_t tv;
    pthread_cond_init(&tv, NULL);

    pthread_mutex_t t_mutex;
    pthread_mutex_init(&t_mutex, NULL);

    int ready = 0;

    long current_reference_timestamp = 0;

    timer_args *t_args = initialize_timer_args(60, &tv, &current_reference_timestamp, &t_mutex, &ready);
    start_timer(t_args);

    printf("Moving average timer started successfully\n");

    // main working task
    while (1) {

        pthread_mutex_lock(&t_mutex);
        while (!ready) {
            pthread_cond_wait(&tv, &t_mutex);
        }
        ready = 0;
        pthread_mutex_unlock(&t_mutex);

        // printf("[*] Logging moving average\n");

        pthread_mutex_lock(&values_mut);
        for (int i = 0; i < n; i++) {

            if (values[i].list != NULL) {
                // clear all old items
                values[i].list = clear_old_items(values[i].list, ma_args->delay);

                double moving_average = 0;
                double total_volume = 0;
                int number_of_datapoints = 0;

                Node *ptr = values[i].list;

                if (ptr != NULL) {

                    while (ptr != NULL) {
                        moving_average += ptr->value;
                        total_volume += ptr->volume;
                        number_of_datapoints++;

                        ptr = ptr->next;
                    }

                    if (number_of_datapoints > 0) {
                        moving_average = moving_average / number_of_datapoints;
                    }

                    // write the moving average to the respective file
                    // get timestamp here and log it as logging timestamp
                    struct timeval logging_timestamp;
                    gettimeofday(&logging_timestamp, NULL);
                    long current_logging_timestamp = logging_timestamp.tv_sec * 1000 + logging_timestamp.tv_usec / 1000;

                    fprintf(ma_args->fc_ma->files[i].f, "%f, %f, %ld, %ld\n", moving_average, total_volume, current_reference_timestamp, current_logging_timestamp);
                }
            }
        }

        pthread_mutex_unlock(&values_mut);
    }

    return NULL;
}