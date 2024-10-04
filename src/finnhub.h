#ifndef FINNHUB_H
#define FINNHUB_H

#include <pthread.h>

#include "queue.h"

typedef struct {
    const char** provided_tickers;
    int provided_tickers_n;
    const char* api_key;
    queue* q;
    int* waiting_for_execution;
} finnhub_args;

// functions to initialize and cleanup finnhub thread worker arguments
finnhub_args* initialize_finnhub_args(const char** provided_tickers, int provided_tickers_n, const char* api_key, queue* q, int* waiting_for_execution);
void delete_finnhub_args(finnhub_args* f_args);

// function to run finnhub worker thread
void* finnhub_worker(void* args);

// function to handle the signal
void sigint_handler(int sig);

#endif