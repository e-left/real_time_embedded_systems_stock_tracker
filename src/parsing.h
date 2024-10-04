#include <string.h>

#include "queue.h"

typedef struct {
    char** data_messages;
    int n;
} parsing_results;

parsing_results* extract_data_messages(char* message);
queue_value extract_data(char* data_message, long received_timestamp);
void clear_parsing_results(parsing_results* p);