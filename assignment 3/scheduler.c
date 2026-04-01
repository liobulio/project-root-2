#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "scheduler.h"
#include "ready_queue.h"
#include "shellmemory.h"
#include "shell.h" 

extern ready_queue *my_queue;

#define POLICY_FCFS 1
#define POLICY_SJF 2
#define POLICY_RR 3
#define POLICY_AGING 4
#define POLICY_RR30 5

// for multithread 
pthread_t workers[2]; // array of IDs of two worker threads

int mt_enabled = 0;
int threads_initialized = 0;
int system_running = 1; // 0 = exit	
int active_workers = 0; //tracks if there's any process running
int current_mt_policy = POLICY_RR; //keeps track of policy RR or RR30

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // the mutual exclusion lock

// the worker thread function
//release the lock before executing parseInt to achieve multithreading
void* worker_routine(void* arg) {

	//keep looking for work as long as the OS is alive
    while(system_running) {
        //grab the lock, if the other one has it, we will freeze here and wait
        pthread_mutex_lock(&queue_mutex);

		
        if (my_queue == NULL || my_queue->head == NULL) {
			//if the queue is empty, drop the clock so the other thread can use it
            pthread_mutex_unlock(&queue_mutex);
            usleep(1000); //sleep briefly if no work
            continue;
        }
        
        //grab the first process if the queue is not empty
        PCB *current = dequeue(my_queue);
		active_workers++;
		//drop the lock so the other thread can grab another process
        pthread_mutex_unlock(&queue_mutex);

        //concurrent execution and the lock has been removed earlier
        int time_slice = (current_mt_policy == POLICY_RR) ? 2 : 30;
        int end_position = current->start_position + current->length;
        int lines_run = 0;

        while (lines_run < time_slice && current->pc_instruction_index < end_position) {
            char *line = mem_get_line(current->pc_instruction_index);
            parseInput(line);
            current->pc_instruction_index++;
            lines_run++;
        }

        //lock the queue again to return the process or clean it up
        pthread_mutex_lock(&queue_mutex);
        if (current->pc_instruction_index >= end_position) { //program finished
            int final_line_index = current->start_position + current->length - 1;
            mem_cleanup_script(current->start_position, final_line_index);
            free(current);
        } else { //program got interrupted
            enqueue_fifo(my_queue, current);
        } 
		active_workers--;
        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}


//called by exec() if user types in 'MT'
void enable_multithreading() {
    if (!threads_initialized) {
        mt_enabled = 1;
        threads_initialized = 1;
        system_running = 1;
        // Boot up the two worker cores!
        pthread_create(&workers[0], NULL, worker_routine, NULL);
        pthread_create(&workers[1], NULL, worker_routine, NULL);
    }
}

void join_threads() {
    if (threads_initialized) {
         
        //only join if we are the main thread (prevents deadlock)
        if (pthread_self() != workers[0] && pthread_self() != workers[1]) {
            system_running = 0;
			pthread_join(workers[0], NULL);
            pthread_join(workers[1], NULL);
        } else {
			while(1) {
                pthread_mutex_lock(&queue_mutex);
                
                //if the queue is empty and i'm the only active worker(thread) left:
                if ((my_queue == NULL || my_queue->head == NULL) && active_workers == 1) {
                    pthread_mutex_unlock(&queue_mutex);
                    break;
                }
                
                pthread_mutex_unlock(&queue_mutex);
                usleep(1000);
            }
		}
    }
}

int scheduler(int policy_code) {

	//multithreaded
	if (mt_enabled) {
        current_mt_policy = policy_code;
        
        while(1) {
            pthread_mutex_lock(&queue_mutex);
            if ((my_queue == NULL || my_queue->head == NULL) && active_workers == 0) {
                pthread_mutex_unlock(&queue_mutex);
                break;
            }
            pthread_mutex_unlock(&queue_mutex);
            usleep(1000);
        }
        return 0;
    }
    
    //non-preemptive, run until finished
    if (policy_code == POLICY_FCFS || policy_code == POLICY_SJF) {
        
        while (my_queue->head != NULL) {
            PCB *current = dequeue(my_queue); //get the first pcb available

           
            int end_position = current->start_position + current->length;
          
            //run until finished
            //run until the pc goes past the end of the script
            while (current->pc_instruction_index < end_position) {
                //fetch the next line of code from memory
                char *line = mem_get_line(current->pc_instruction_index);
                
                //execute
                parseInput(line); 
                
                //move to the next instruction
                current->pc_instruction_index++; 
            }

            //free the pointers and script lines when the process finished
            if (current->pc_instruction_index >= end_position) {
              int final_line_index = current->start_position + current->length -1;
              mem_cleanup_script(current->start_position, final_line_index);

              //clean up the pcb
              free(current);
            }
        }
    }
    
    else if (policy_code == POLICY_RR || policy_code == POLICY_AGING || policy_code == POLICY_RR30 ) {

        //time slice for rr is 2 instructions, and 1 for aging
        int time_slice;
        if (policy_code == POLICY_RR) {
            time_slice = 2;
        }
        else if (policy_code == POLICY_AGING) {
            time_slice = 9999;
        }
        else {
            time_slice = 30;
        }

        while (my_queue->head != NULL) {
            PCB *current = dequeue(my_queue);

            int end_position = current->start_position + current->length;
            int lines_run = 0; //lines that had been run

	    //runs until finished or get preempted
            while (lines_run < time_slice && current->pc_instruction_index < end_position) {

                char *line = mem_get_line(current->pc_instruction_index);
                parseInput(line);

                current->pc_instruction_index++;
                lines_run++;
            

            if (policy_code == POLICY_AGING) {

				//age the queue
                PCB *temp = my_queue->head;
                while (temp != NULL) {
                    if (temp->job_length_score > 0) {
                        temp->job_length_score--;
                    }
                    temp = temp->next;
                }
	    		if (my_queue->head != NULL && my_queue->head->job_length_score < current->job_length_score) {
	    			break;
	    		}
            }
		}

            if (current->pc_instruction_index >= end_position) {
                int final_line_index = current->start_position + current->length - 1;
                mem_cleanup_script(current->start_position, final_line_index);
                free(current);
            } else { //interrupted
                if (policy_code == POLICY_RR || policy_code == POLICY_RR30) {
                    enqueue_fifo(my_queue, current);
                } else if (policy_code == POLICY_AGING){
                    enqueue_sorted_by_score(my_queue, current);
                }
            }
        }
    }

    return 0;
}
