#include <stdio.h>
#include <stdlib.h>
#include "linked_list.h"


Queue* join_queue(Queue* head, int customer_id, int service_time, double cur_time){
    Queue* new = (Queue*)malloc(sizeof(Queue));
    new->customer_id = customer_id;
    new->service_time = service_time;
    new->enter_que = cur_time;
    if(head == NULL){
        return new;
    }
    Queue* curr = head;
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = new;
    return head;
}

Queue* pop_front(Queue* head){
    if(head != NULL){
        head = head->next;
        return head;
    }
    return NULL;
}

int head_customer_id(Queue* head){
    return head->customer_id;
}

int head_service_time(Queue* head){
    return head->service_time;
}

double head_queued_time(Queue* head){
    return head->enter_que;
}

int check_empty(Queue* head){
    if(head == NULL){
        return 0;
    }
    return 1;
}

int id_in_queue(Queue* head, int id){
    if(head == NULL){
        return 0;
    }
    Queue* curr = head;
    while(curr != NULL){
        if(curr->customer_id == id){
            return 1;
        }
        curr =  curr->next;
    }
    return 0;
}

int queue_length(Queue* head){
    if(head == NULL){
        return 0;
    }
    Queue* curr = head;
    int i = 0;
    while(curr != NULL){
        curr = curr->next;
        i++;
    }
    return i;
}