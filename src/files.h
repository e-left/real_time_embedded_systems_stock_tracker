#ifndef FILES_H
#define FILES_H

#include <stdio.h>

#define OUTPUT_FOLDER "./out/"

typedef struct {
    FILE* f;
    const char* symbol;
} ticker_file;

typedef struct {
    ticker_file* files;
    int n;
} file_collection;

file_collection* initialize_file_collection(const char** provided_tickers, int provided_tickers_n, const char* collection_name);
void close_file_connection(file_collection* fc);

#endif