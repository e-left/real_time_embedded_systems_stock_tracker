#ifndef LINKEDLIST_H
#define LINKEDLIST_H

struct Node{
    struct Node* next;
    double value;
    double volume;
    long timestamp;
}; 

typedef struct Node Node;

Node* add_list_item(Node* head, double value, double volume, long timestamp);
Node* clear_old_items(Node* head, long earliest_timepoint_seconds);
Node* get_most_recent_items(Node* head, long earliest_timepoint_seconds);
void delete_list(Node* head);

#endif