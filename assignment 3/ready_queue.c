//
// Created by billz on 2/21/2026.
//
# include <stdlib.h>
# include "ready_queue.h"

ready_queue *create_queue() {

    ready_queue *queue = malloc(sizeof(ready_queue));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;

}

void destroy_queue(ready_queue *queue) {

    PCB *temp = queue->head;

    while (temp != NULL) {
        PCB *prev = temp;
        temp = temp->next;
        free(prev);
    }

    free(queue);
}

PCB *dequeue(ready_queue *queue) {

    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    PCB *temp = queue->head;
    queue->head = queue->head->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    temp->next = NULL;

    return temp;
}

void enqueue_fifo(ready_queue *queue, PCB *new_pcb) {

    // safety check if input is empty, just do nothing
    if (queue == NULL || new_pcb == NULL) {
        return;
    }

    // if queue is empty
    if (queue->head == NULL) {
        queue->head = new_pcb;
        queue->tail = new_pcb;
        new_pcb->next = NULL;
        return;
    }

    // update normally to the tail
    // PCB *temp = queue->tail;
    queue->tail->next = new_pcb;
    queue->tail = new_pcb;
    new_pcb->next = NULL;

}

// suppose the queue is sorted already
void enqueue_sorted_by_length(ready_queue *queue, PCB *new_pcb) {

    // safety check if input is empty, just do nothing
    if (queue == NULL || new_pcb == NULL) {
        return;
    }

    // if queue is empty
    if (queue->head == NULL) {
        queue->head = new_pcb;
        queue->tail = new_pcb;
        new_pcb->next = NULL;
        return;
    }

    // if new_pcb's length is the smallest, then update head
    if (new_pcb->length < queue->head->length) {
        PCB *temp = queue->head;
        queue->head = new_pcb;
        queue->head->next = temp;
        return;
    }

    // if new_pcb's length is at the middle, update it and enqueue it to the middle
    PCB *temp = queue->head;
    while (temp->next != NULL) {
        if (temp->next->length > new_pcb->length) {
            new_pcb->next = temp->next;
            temp->next = new_pcb;
            return;
        }
        temp = temp->next;
    }

    // if new_pcb's length is the largest, update tail
    temp->next = new_pcb;
    new_pcb->next = NULL;
    if (new_pcb->next == NULL) {
        queue->tail = new_pcb;
    }

}

void enqueue_sorted_by_score(ready_queue *queue, PCB *new_pcb) {

    // safety check if input is empty, just do nothing
    if (queue == NULL || new_pcb == NULL) {
        return;
    }

    // if queue is empty
    if (queue->head == NULL) {
        queue->head = new_pcb;
        queue->tail = new_pcb;
        new_pcb->next = NULL;
        return;
    }
	// if new_pcb's score is the smallest, then update head
    if (new_pcb->job_length_score < queue->head->job_length_score) {
        PCB *temp = queue->head;
        queue->head = new_pcb;
        queue->head->next = temp;
        return;
    }

    // if new_pcb's score is at the middle, update it and enqueue it to the middle
    PCB *temp = queue->head;
    while (temp->next != NULL) {
        if (temp->next->job_length_score > new_pcb->job_length_score) {
            new_pcb->next = temp->next;
            temp->next = new_pcb;
            return;
        }
        temp = temp->next;
    }

    // if new_pcb's score is the largest, update tail
    temp->next = new_pcb;
    new_pcb->next = NULL;
    if (new_pcb->next == NULL) {
        queue->tail = new_pcb;
    }
}

// special enqueue for background which must let batch stay in head
void enqueue_sorted_by_length_with_batch(ready_queue *queue, PCB *new_pcb) {

    // safety check if input is empty, just do nothing
    if (queue == NULL || new_pcb == NULL) {
        return;
    }
	// fix head which is batch
    PCB *batch = queue->head;
	if(batch->next == NULL) {
		batch->next = new_pcb;
        new_pcb->next = NULL;
		queue->tail = new_pcb;
		return;
	}

	PCB *temp = batch->next;
    // if new_pcb's length is at the middle, update it and enqueue it to the middle
    while (temp->next != NULL) {
        if (temp->next->length > new_pcb->length) {
            new_pcb->next = temp->next;
            temp->next = new_pcb;
            return;
        }
        temp = temp->next;
    }

    // if new_pcb's length is the largest, update tail
    temp->next = new_pcb;
    new_pcb->next = NULL;
    if (new_pcb->next == NULL) {
        queue->tail = new_pcb;
    }
}

// special enqueue for background which must let batch stay in head
void enqueue_sorted_by_score_with_batch(ready_queue *queue, PCB *new_pcb) {

    // safety check if input is empty, just do nothing
    if (queue == NULL || new_pcb == NULL) {
        return;
    }
	// fix head which is batch
    PCB *batch = queue->head;
	if(batch->next == NULL) {
		batch->next = new_pcb;
        new_pcb->next = NULL;
		queue->tail = new_pcb;
		return;
	}

	PCB *temp = batch->next;
    // if new_pcb's job_length_score is at the middle, update it and enqueue it to the middle
    while (temp->next != NULL) {
        if (temp->next->job_length_score > new_pcb->job_length_score) {
            new_pcb->next = temp->next;
            temp->next = new_pcb;
            return;
        }
        temp = temp->next;
    }

    // if new_pcb's job_length_score is the largest, update tail
    temp->next = new_pcb;
    new_pcb->next = NULL;
    if (new_pcb->next == NULL) {
        queue->tail = new_pcb;
    }
}

