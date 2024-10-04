#include <libwebsockets.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "finnhub.h"
#include "queue.h"
#include "parsing.h"

#define N_RETRIES 10

static int stopped = 0;
static struct lws *client_wsi = NULL;
char **tickers;
int tickers_n = 0;

queue* q;

static int callback_finnhub(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        lwsl_user("Connected to Finnhub\n");
        stopped = 0;
        // Send subscription message to WebSocket
        // Loop over and subscribe to all specified tickers
        for (int i = 0; i < tickers_n; i++) {
            unsigned char msg[LWS_PRE + 64];
            int n = sprintf((char *)msg + LWS_PRE, "{\"type\":\"subscribe\",\"symbol\":\"%s\"}", tickers[i]);
            lws_write(wsi, msg + LWS_PRE, n, LWS_WRITE_TEXT);
        }
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        // parse provided data
        char* data = (char *) in;
        // only go in if the word data is present
        if (strstr(data, "data") == NULL) {
            break;
        }

        // get timestamp here and log it later as data received timestamp
        struct timeval received_timestamp;
        gettimeofday(&received_timestamp, NULL);
        long current_received_timestamp = received_timestamp.tv_sec * 1000 + received_timestamp.tv_usec / 1000;

        parsing_results* p_res = extract_data_messages(data);

        // parse and send results to queue
        for (int i = 0; i < p_res->n; i++) {
            queue_value q_val = extract_data(p_res->data_messages[i], current_received_timestamp);
            pthread_mutex_lock(q->mut);
            while (q->full) {
                pthread_cond_wait(q->notFull, q->mut);
            }
            queueAdd(q, q_val);
            pthread_mutex_unlock(q->mut);
            pthread_cond_signal(q->notEmpty);
        }

        // cleanup
        clear_parsing_results(p_res);
        break;
}
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        lwsl_err("Connection error: %s\n", in ? (char *)in : "(null)");
        stopped = 1;
        break;
    case LWS_CALLBACK_CLIENT_CLOSED:
        lwsl_user("Disconnected from server\n");
        stopped = 1;
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "finnhub-protocol",
        callback_finnhub,
        0,
        4096,
    },
    {NULL, NULL, 0, 0} /* terminator */
};

void setup_tickers(const char **provided_tickers, int provided_tickers_n) {
    tickers_n = provided_tickers_n;
    tickers = (char **)malloc(sizeof(char *) * provided_tickers_n);
    for (int i = 0; i < tickers_n; i++) {
        tickers[i] = (char *)malloc(sizeof(char) * 32); // Ensure we allocate enough space for any ticker
        strcpy(tickers[i], provided_tickers[i]);
    }
}

void setup_queue(queue* provided_queue) {
    q = provided_queue;
}

void cleanup_finnhub() {
    for (int i = 0; i < tickers_n; i++) {
        free(tickers[i]);
    }
    free(tickers);
}

finnhub_args* initialize_finnhub_args(const char** provided_tickers, int provided_tickers_n, const char* api_key, queue* q, int* waiting_for_execution) {
    finnhub_args* f_args = (finnhub_args*)malloc(sizeof(finnhub_args));
    if (f_args == NULL) {
        printf("Error initializing finnhub arguments struct. Exiting... \n");
        exit(1);
    }

    f_args->provided_tickers = provided_tickers;
    f_args->provided_tickers_n = provided_tickers_n;
    f_args->api_key = api_key;
    f_args->q = q;
    f_args->waiting_for_execution = waiting_for_execution;

    return f_args;
}

void delete_finnhub_args(finnhub_args* f_args) {
    free(f_args);
}

// function that will run the thread for the finnhub worker
void* finnhub_worker(void* args) {
    //  extract arguments
    finnhub_args* f_args = (finnhub_args*) args;
    const char** provided_tickers = f_args->provided_tickers;
    int provided_tickers_n = f_args->provided_tickers_n;
    const char* api_key = f_args->api_key;
    queue* f_queue = f_args->q;

    // set up tickers
    setup_tickers(provided_tickers, provided_tickers_n);

    // set up queue
    setup_queue(f_queue);

    // set up connection
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;
    struct lws *wsi;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    // Alter this for initial logging changes
    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, NULL);

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        exit(1);
    }

    memset(&ccinfo, 0, sizeof ccinfo);
    ccinfo.context = context;
    ccinfo.address = "ws.finnhub.io";
    ccinfo.port = 443;
    char path[256];
    snprintf(path, sizeof(path), "/?token=%s", api_key);
    ccinfo.path = path;
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL;

    wsi = lws_client_connect_via_info(&ccinfo);
    while (!stopped) {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);

    cleanup_finnhub();

    *(f_args->waiting_for_execution) = 0;

    return(NULL);
}