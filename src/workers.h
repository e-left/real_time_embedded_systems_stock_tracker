#ifndef WORKERS_H
#define WORKERS_H

#include "queue.h"
#include "files.h"

typedef struct {
    queue* q;
    file_collection* fc_prices;
} logger_args;

typedef struct {
    long delay;
    file_collection* fc_ma;
} moving_average_args;

typedef struct {
    long delay;
    file_collection* fc_c;
} candlestick_args;

void setup_workers(const char** provided_tickers, int provided_tickers_n);
void clear_workers();

logger_args* initialize_logger_args(queue* q, file_collection* fc_prices);
void delete_logger_args(logger_args* l_args);
void* logger_worker(void* args);

moving_average_args* initialize_moving_average_args(long delay, file_collection* fc_ma);
void delete_moving_average_args(moving_average_args* ma_args);
void* moving_average_worker(void* args);

candlestick_args* initialize_candlestick_args(long delay, file_collection* fc_c);
void delete_candlestick_args(candlestick_args* c_args);
void* candlestick_worker(void* args);

#endif