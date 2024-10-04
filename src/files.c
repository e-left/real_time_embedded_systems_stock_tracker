#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"


file_collection* initialize_file_collection(const char** provided_tickers, int provided_tickers_n, const char* collection_name) {
    // create output folder if it does not exist
    struct stat st = {0};

    if (stat(OUTPUT_FOLDER, &st) == -1) {
        mkdir(OUTPUT_FOLDER, 0755);
    }

    file_collection* fc = (file_collection*) malloc(sizeof(file_collection));
    if (fc == NULL) {
        printf("Error initializing file collection. Exiting...\n");
        exit(1);
    }
    fc->n = provided_tickers_n;

    fc->files = (ticker_file*) malloc(fc->n * sizeof(ticker_file));
    if (fc->files == NULL) {
        printf("Error initializing file collection ticker files. Exiting...\n");
        exit(1);
    }

    for (int i = 0; i < fc->n; i++) {
        struct timeval timestamp;
        gettimeofday(&timestamp, NULL);
        long current_timestamp = timestamp.tv_sec;
        char timestamp_str[50] = "";
        sprintf(timestamp_str, "%ld", current_timestamp);

        char filename[200] = "";

        strcat(filename, OUTPUT_FOLDER);
        strcat(filename, collection_name);
        strcat(filename, "_");
        strcat(filename, timestamp_str);
        strcat(filename, "_");
        strcat(filename, provided_tickers[i]);
        strcat(filename, ".txt");
        
        fc->files[i].symbol = provided_tickers[i];
        fc->files[i].f = fopen(filename, "w");
    }

    return fc;
}

void close_file_connection(file_collection* fc) {
    for (int i = 0; i < fc->n; i++) {
        fclose(fc->files[i].f);
    }

    free(fc->files);
    free(fc);
}