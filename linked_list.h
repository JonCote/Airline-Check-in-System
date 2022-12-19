#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

typedef struct Queue Queue;

struct Queue{
    int customer_id;
    int service_time;
    double enter_que;
    Queue* next;
};

Queue* join_queue(Queue* head, int customer_id, int service_time, double cur_time);
Queue* pop_front(Queue* head);
int head_customer_id(Queue* head);
int head_service_time(Queue* head);
double head_queued_time(Queue* head);
int check_empty(Queue* head);
int id_in_queue(Queue* head, int id);
int queue_length(Queue* head);

#endif