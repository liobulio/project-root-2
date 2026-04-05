#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "pcb.h"

typedef struct ready_queue {

    PCB *head;
    PCB *tail;
} ready_queue;

//ready queue operation 
ready_queue *create_queue();
void destroy_queue(ready_queue *queue);
PCB *dequeue(ready_queue *queue);
void enqueue_fifo(ready_queue *queue, PCB *new_pcb);              //first in first out 
void enqueue_sorted_by_length(ready_queue *queue, PCB *new_pcb);  //shorted job first
void enqueue_sorted_by_score(ready_queue *queue, PCB *new_pcb);   //aging
void enqueue_sorted_by_length_with_batch(ready_queue *queue, PCB *new_pcb);  //shorted job first with background
void enqueue_sorted_by_score_with_batch(ready_queue *queue, PCB *new_pcb);   //aging with background

#endif
