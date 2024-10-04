#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "finnhub.h"
#include "queue.h"
#include "workers.h"
#include "files.h"

#define MOVING_AVERAGE_DELAY_S 60*15
#define CANDLESTICK_DELAY_S 60

int wait_for_execution = 1;

void sigint_handler(int sig) {
    wait_for_execution = 0;
}

int main(int argc, const char **argv) {
    // ensure arguments are passed
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Finnhub API Key> <Ticker 1> [Ticker 2]\n", argv[0]);
        return 1;
    }

    // grab API key
    const char *api_key = argv[1];
    // grab provided tickers
    const char **provided_tickers = &(argv[2]);
    int provided_tickers_n = argc - 2;

    // set up files
    file_collection* fc_prices = initialize_file_collection(provided_tickers, provided_tickers_n, "PRICES");
    file_collection* fc_ma = initialize_file_collection(provided_tickers, provided_tickers_n, "MA");
    file_collection* fc_c = initialize_file_collection(provided_tickers, provided_tickers_n, "C");

    // set up thread arguments

    // communication queue between finnhub thread and logger thread
    queue *q = queueInit();
    if (q == NULL) {
        printf("Error initializing queue. Exiting...");
        exit(1);
    }

    setup_workers(provided_tickers, provided_tickers_n);

    // finnhub thread arguments
    finnhub_args *f_args = initialize_finnhub_args(provided_tickers, provided_tickers_n, api_key, q, &wait_for_execution);

    // logger thread arguments
    logger_args* l_args = initialize_logger_args(q, fc_prices);

    // moving average thread arguments
    moving_average_args* ma_args = initialize_moving_average_args(MOVING_AVERAGE_DELAY_S, fc_ma);

    // candlestick thread arguments
    candlestick_args* c_args = initialize_candlestick_args(CANDLESTICK_DELAY_S, fc_c);

    // initialize finnhub thread
    pthread_t finnhub_t;

    // initialize logger thread
    pthread_t logger_t;

    // initialize moving average thread
    pthread_t ma_t;

    // initialize candlestick thread
    pthread_t c_t;

    // register signal to stop execution
    signal(SIGINT, sigint_handler);

    // start all threads
    pthread_create(&finnhub_t, NULL, finnhub_worker, (void *)f_args);
    pthread_create(&logger_t, NULL, logger_worker, (void *)l_args);
    pthread_create(&ma_t, NULL, moving_average_worker, (void*) ma_args);
    pthread_create(&c_t, NULL, candlestick_worker, (void*) c_args);

    // wait for threads to finish
    // pthread_join(finnhub_t, NULL); // No need, since the thread will exit
    while(wait_for_execution) {
        sleep(5); 
    }

    // Ensure that the queue is empty before exiting
    while(!q->empty) {
        printf("Queue still has items, waiting for processing...");
        sleep(5);
    }

    printf("Stopping...\n");

    // wait only for that thread since the rest don't have value if that one is not running
    // cancel when waiting for the queue
    pthread_cancel(logger_t);
    pthread_cancel(ma_t);
    pthread_cancel(c_t);

    printf("Stopped. Cleaning up...\n");

    pthread_join(logger_t, NULL);
    pthread_join(ma_t, NULL);
    pthread_join(c_t, NULL);

    // cleanup
    queueDelete(q);
    delete_finnhub_args(f_args);
    delete_logger_args(l_args);
    delete_moving_average_args(ma_args);
    delete_candlestick_args(c_args);
    clear_workers();

    // closing files
    close_file_connection(fc_prices);
    close_file_connection(fc_ma);
    close_file_connection(fc_c);

    printf("Cleaned up. Exiting...\n");

    return 0;
}
