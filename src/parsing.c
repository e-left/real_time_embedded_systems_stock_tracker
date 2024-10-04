#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parsing.h"
#include "queue.h"

parsing_results *extract_data_messages(char *message) {
    // get index of first occurence of [
    char *beginning = strchr(message, '[');

    // get index of last occurence of ]
    char *ending = strrchr(message, ']');

    // also, remove first [ and last ]
    beginning += 1;
    ending -= 1;

    // get substring between them
    int substring_length = ending - beginning + 1;
    char substring[substring_length + 1]; // null byte
    memset(substring, '\0', substring_length + 1);
    strncpy(substring, beginning, substring_length);

    // first, count the number of { characters
    int n = 0; // start from 1
    for (int i = 0; i < strlen(substring); i++) {
        if (substring[i] == '{') {
            n++;
        }
    }

    // initialize parsing results
    parsing_results *res = (parsing_results *)malloc(sizeof(parsing_results));
    if (res == NULL) {
        printf("Error while initializing parsing results. Exiting...");
        exit(1);
    }

    res->n = n;
    res->data_messages = (char **)malloc(n * sizeof(char *));
    if (res->data_messages == NULL) {
        printf("Error while initializing parsing results data messages. Exiting...");
        exit(1);
    }

    char *message_extracting = (char *)malloc((strlen(substring) + 1) * sizeof(char)); // account for null byte
    if (message_extracting == NULL) {
        printf("Error while initializing message extracting variable. Exiting...");
        exit(1);
    }

    strcpy(message_extracting, substring);

    for (int i = 0; i < n; i++) {
        // get index of first occurence of {
        char *beginning = strchr(message_extracting, '{');
        // get index of first occurence of }
        char *ending = strchr(message_extracting, '}');

        // get substring between them
        int this_message_length = ending - beginning + 1 - 2; // skip { and }
        char this_message[this_message_length + 1];
        memset(this_message, '\0', this_message_length + 1);
        strncpy(this_message, beginning + 1, this_message_length);

        // copy it to result
        res->data_messages[i] = (char *)malloc((strlen(this_message) + 1) * sizeof(char)); // acount for null byte
        if (res->data_messages[i] == NULL) {
            printf("Error while initializing parsing results data message. Exiting...");
            exit(1);
        }
        strcpy(res->data_messages[i], this_message);

        // update substring
        message_extracting = ending + 2; // skip characters },
    }

    return res;
}

char *extract_x_substring(char *input, char desc) {
    // will find all 4 substrings using this clever method
    char search_str[5];
    sprintf(search_str, "\"%c\":", desc);
    char *x_substring_beginning = strstr(input, search_str);
    char *x_substring_end = strchr(x_substring_beginning, ',');

    int x_substring_length;
    if (x_substring_end == NULL) {
        x_substring_length = strlen(x_substring_beginning);
    } else {
        x_substring_length = x_substring_end - x_substring_beginning;
    }

    char *x_substring = (char*) malloc((x_substring_length + 1) * sizeof(char));
    if (x_substring == NULL) {
        printf("Error allocating x substring. Exiting...");
        exit(1);
    }
    memset(x_substring, '\0', x_substring_length + 1);
    strncpy(x_substring, x_substring_beginning, x_substring_length);

    return x_substring;
}

queue_value extract_data(char *data_message, long received_timestamp) {
    // message has the form
    // "c":null,"p":62643.98,"s":"BINANCE:BTCUSDT","t":1721037602481,"v":0.00033
    // or
    // "c":["1","24","12"],"p":235.1,"s":"AAPL","t":1721037599412,"v":2
    double value;
    double volume;
    char *symbol;
    long timestamp;

    char* p_substring = extract_x_substring(data_message, 'p');
    char* s_substring = extract_x_substring(data_message, 's');
    char* t_substring = extract_x_substring(data_message, 't');
    char* v_substring = extract_x_substring(data_message, 'v');

    value = atof(p_substring + 4);
    volume = atof(v_substring + 4);
    int symbol_len = strlen(s_substring) - 4 - 2;
    symbol = (char*) malloc((symbol_len + 1) * sizeof(char));
    if (symbol == NULL) {
        printf("Error allocating symbol. Exiting...");
        exit(1);
    }
    memset(symbol, '\0', symbol_len + 1);
    strncpy(symbol, s_substring + 5, symbol_len);
    timestamp = atol(t_substring + 4);

    // package and return result
    queue_value q;

    q.value = value;
    q.volume = volume;
    q.symbol = symbol;
    q.timestamp = timestamp;
    q.received_timestamp = received_timestamp;

    // cleanup 
    free(p_substring);
    free(s_substring);
    free(v_substring);
    free(t_substring);

    return q;
}

void clear_parsing_results(parsing_results *p) {
    for (int i = 0; i < p->n; i++) {
        free(p->data_messages[i]);
    }
    free(p->data_messages);
    free(p);
}