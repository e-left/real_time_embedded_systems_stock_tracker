#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "linkedlist.h"

Node* add_list_item(Node* head, double value, double volume, long timestamp) {
    Node* node = (Node*) malloc(sizeof(Node));
    if (node == NULL) {
        printf("Error initializing linked list. Exiting...\n");
        exit(1);
    }

    node->value = value;
    node->volume = volume;
    node->timestamp = timestamp;
    node->next = NULL;

    if (head == NULL) {
        head = node;
        return head;
    }

    Node* t = head;

    while (t->next != NULL) {
        t = t->next;
    }

    t->next = node;

    return head;
}

// Data is real time, so this is sufficient (assuming they come in order)
Node* clear_old_items(Node* head, long earliest_timepoint_seconds) {
    // used to clear items that are older than 15 minutes

    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    long current_timestamp = timestamp.tv_sec * 1000 + timestamp.tv_usec / 1000;

    Node* previous = head;
    Node* new_head = head;
    while(previous != NULL && current_timestamp - previous->timestamp > earliest_timepoint_seconds * 1000) {
        previous = new_head;
        if (previous != NULL) {
            new_head = previous->next;
        }
    }

    if(new_head == head) {
        return head;
    }

    // here previous might be null
    if (previous != NULL) {
        previous->next = NULL;
    }
    delete_list(head);

    return new_head;
}

// Data is real time, so this is sufficient
Node* get_most_recent_items(Node* head, long earliest_timepoint_seconds) {
    // used to get minute recent data

    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    long current_timestamp = timestamp.tv_sec * 1000 + timestamp.tv_usec / 1000;

    Node* ptr = head;
    while(ptr != NULL && current_timestamp - ptr->timestamp > 1000 * earliest_timepoint_seconds) {
        ptr = ptr->next;
    }

    return ptr;
}

void delete_list(Node* head) {
    while (head != NULL) {
        Node* next = head->next;
        free(head);
        head = next;
    }
}