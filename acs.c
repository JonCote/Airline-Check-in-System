/*
* CSC 360 A2, ASC system using multi-threading
* Author: Jonathan Cote V00962634
*/


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include "linked_list.h"
#include "customer.h"
#define CLERK_COUNT 5


typedef struct Clerk{
    int id;
    int state;
} Clerk;

struct timeval init_time;
double init_secs;
double overall_waiting_time;
pthread_mutex_t econQueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t busQueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customerArrived = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customerServed = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t busCustomerQueued = PTHREAD_COND_INITIALIZER;
pthread_cond_t econCustomerQueued = PTHREAD_COND_INITIALIZER;

Queue *econQueue;
Queue *busQueue;
Clerk clerks[CLERK_COUNT];
Customer *customer_info = NULL;
int econQueueLength;
int busQueueLength;

int customers_served = 0;
int total_customers;
int customer_arrival[2] = {0, 0};


/*
 * Function: mem_alloc
 * --------------------------------
 * allocate memory for the customer structs based on how many customers are expected
 * 
 * customer_info: the pointer of the customer info starting location
 * size: the number of customers expected, used to make appropriate size
 * 
*/
Customer* mem_alloc(Customer *customer_info, int size){
    Customer *temp = NULL;
    if(customer_info == NULL){
        temp = (Customer *) malloc(sizeof(Customer)*size);
        return temp;
    }
    else{
        temp = (Customer *)realloc(customer_info, sizeof(Customer)*size);
        
        if(temp != NULL){
            return temp;
        }
        else{
            printf("realloc failed");
            exit(1);
        }
    }
}


/*
 * Function: open_file
 * --------------------------------
 * open up the specified file
 * 
 * filename: file name to be opened
 * 
 * return: FILE* pointer to opened file
*/
FILE* open_file(char *filename){
    FILE *fptr = fopen(filename, "r");
    if(fptr == NULL){
        printf("File not found\n");
        exit(1);
    }
    return fptr;
}


/*
 * Function: read_customer_info
 * --------------------------------
 * parse the input file to gather relevant customer info
 * 
 * file: file to be parsed
 * numOfCustomers: pointer to the global that stores the number of customers
 * 
 * return: pointer to start of the customer info set
*/
Customer* read_customer_info(FILE *file, int *numOfCustomers){
    char line[160];
    int customer_id;
    int class;
    int arrival_time;
    int service_time;
    int i = 0;
    int ignore = 0;
    Customer *customer_info = NULL;

    while(fgets(line, 160, file) != NULL){
        if(i == 0){
            sscanf(line, "%d", numOfCustomers);
            customer_info = mem_alloc(customer_info, *numOfCustomers);
            total_customers = *numOfCustomers;
            i++;
            
        }
        else{
            sscanf(line, "%d:%d,%d,%d", &customer_id, &class, &arrival_time, &service_time);
            for(int k = 0; k < i; k++){
                if(customer_info[k].id == customer_id){
                    printf("customer with id %d already added, customer info entry will be ignored\n", customer_id);
                    ignore = 1;
                    total_customers--;
                    break;
                }
            }
             
            if((class > 1 || class < 0) && ignore == 0){
                printf("invalid customer class, Customer with id %d will not be added!\n", customer_id);
                ignore = 1;
                total_customers--;
            }
            else if(arrival_time < 0 && ignore == 0){
                printf("Invalid customer arrival time, Customer with id %d will not be added", customer_id);
                ignore = 1;
                total_customers--;
            }
            else if(service_time < 0 && ignore == 0){
                printf("Invalid customer service time, Customer with id %d will not be added!", customer_id);
                ignore = 1;
                total_customers--;
            }
            else if(ignore == 0){
                int j = i-1;
                customer_info[j].id = customer_id;
                customer_info[j].class_type = class;
                customer_info[j].service_time = service_time;
                customer_info[j].arrival_time = arrival_time;
                i++;
            }
            
        }
        ignore = 0;
        
    }
    return customer_info;

}

/*
 * Function: getCurrSimTime
 * --------------------------------
 * Calculate current simulation time.
 * Code was referenced from the sample_gettimeofday.c file provided with assignment
 * 
 * return: double referencing the current simulation time
*/
double getCurrSimTime(){
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    return (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000) - init_secs;

}


/*
 * Function: set_customer_times
 * --------------------------------
 * set the queued up time and start of service time for a customer
 * 
 * serv_start_time: the simulation time when a customer starts getting service
 * customer_queued_at: the simulation time when a customer joins a queue
 * customer_id: the customers id number
 * 
*/
void set_customer_times(double serv_start_time, double customer_queued_at, int customer_id){
    for(int i = 0; i < total_customers; i++){
        if(customer_info[i].id == customer_id){
            customer_info[i].serv_start_time = serv_start_time;
            customer_info[i].queued_time = customer_queued_at;
        }
    }
}


/*
 * Function: get_avg_wait
 * --------------------------------
 * calculates the average wait time for arrival tell service starts for all customers, business class, and economy class customers.
 * 
 * attr: defines the type of customer you want to calcuate average for. 
 *          attr = 2 calculates all customers
 *          attr = 1 calculates business class customers
 *          attr = 0 calculates economy class customers
 * 
 * return: the average wait time for a customer subset
*/
double get_avg_wait(int attr){
    double total_wait;
    double cust_wait;
    int num_customers;
    
    for(int i = 0; i < total_customers; i++){
        if(attr == 2){
            cust_wait = customer_info[i].serv_start_time - customer_info[i].queued_time;
            total_wait = total_wait + cust_wait;
            num_customers++;
        }
        else if(customer_info[i].class_type == attr){
            cust_wait = customer_info[i].serv_start_time - customer_info[i].queued_time;
            total_wait = total_wait + cust_wait;
            num_customers++;
        }
    }

    return total_wait / cust_wait;
}


/*
 * Function: clerk_entry
 * --------------------------------
 * The entry point for a new clerk thread. clerk threads manage and service the customers in the queues
 * Code is semi-referenced from the A2-hint_detailed.c files clerk_entry function
 * 
 * clerk_info: information of the clerk for this given thread
 * 
*/
void* clerk_entry(void * clerk_info){
    Clerk* clerk = (Clerk *)clerk_info;
    int customer_id = -1;
    int customer_serTime = -1;
    double customer_queued_at = 0;
    int control = 1;
    double serv_start_time = 0;
    
    while(control == 1){
        pthread_mutex_lock(&customerServed);
        {
            if(customers_served == total_customers){
                control = 0;
            }
        }
        pthread_mutex_unlock(&customerServed);

        if(busQueueLength > 0){
            pthread_mutex_lock(&busQueueMutex);
            {
                while(customer_arrival[1] == 1){
                    pthread_cond_wait(&busCustomerQueued, &busQueueMutex);
                }
                
                if(busQueueLength > 0){
                    customer_id = head_customer_id(busQueue);
                    customer_serTime = head_service_time(busQueue);
                    customer_queued_at = head_queued_time(busQueue);
                    busQueue = pop_front(busQueue);
                    busQueueLength--;
                }
                
            }
            pthread_mutex_unlock(&busQueueMutex);
        }
        else if(econQueueLength > 0){
            pthread_mutex_lock(&econQueueMutex);
            {
                while(customer_arrival[0] == 1){
                    pthread_cond_wait(&econCustomerQueued, &econQueueMutex);
                }
                
                if(econQueueLength > 0){
                    customer_id = head_customer_id(econQueue);
                    customer_serTime = head_service_time(econQueue);
                    customer_queued_at = head_queued_time(econQueue);
                    econQueue = pop_front(econQueue);
                    econQueueLength--;
                }
                
            }
            pthread_mutex_unlock(&econQueueMutex);
        }

        if(customer_id != -1 && customer_serTime != -1){
            
            serv_start_time = getCurrSimTime();
            printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", serv_start_time, customer_id, clerk->id);

            usleep(customer_serTime * 100000);

            printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", getCurrSimTime(), customer_id, clerk->id);

            pthread_mutex_lock(&customerServed);
            {
                set_customer_times(serv_start_time, customer_queued_at, customer_id);
                customers_served++;
            }
            pthread_mutex_unlock(&customerServed);

            customer_id = -1;
            customer_serTime = -1;
        }
        
    }
    pthread_exit(NULL);

    return NULL;
}


/*
 * Function: customer_entry
 * --------------------------------
 * The entry point for new customer threads. Used to manage customer arrivals and queueing them into respective class type queue
 * Code is semi-referenced from the A2-hint_detailed.c customer_entry function
 * 
 * cust_info: customer's info
 * 
*/
void* customer_entry(void* cust_info){
    Customer *c_info = (Customer *)cust_info;
    
    usleep(c_info->arrival_time * 100000);

    pthread_mutex_lock(&customerArrived);
    {
        printf("A customer arrives: customer ID %2d. \n", c_info->id);
        customer_arrival[c_info->class_type] = 1;

        // put customer into their queue based on class
        if(c_info->class_type == 0){
            pthread_mutex_lock(&econQueueMutex);
            {
                econQueue = join_queue(econQueue, c_info->id, c_info->service_time, getCurrSimTime());
                econQueueLength++;
                printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", c_info->class_type, econQueueLength);

                customer_arrival[c_info->class_type] = 0;
                pthread_cond_broadcast(&econCustomerQueued);
            }
            pthread_mutex_unlock(&econQueueMutex);
        }
        else if(c_info->class_type == 1){
            pthread_mutex_lock(&busQueueMutex);
            {
                busQueue = join_queue(busQueue, c_info->id, c_info->service_time, getCurrSimTime());
                busQueueLength++;
                printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", c_info->class_type, busQueueLength);

                customer_arrival[c_info->class_type] = 0;
                pthread_cond_broadcast(&busCustomerQueued);
            }
            pthread_mutex_unlock(&busQueueMutex);
        }
    }
    pthread_mutex_unlock(&customerArrived);
    
    pthread_exit(NULL);

    return NULL;
}


/*
 * Function: main
 * --------------------------------
 * Entry point into the code. Used to initialize customer info, start needed clerk and customer threads, and call avg wait time calculations
 * Code is reference from the A2-hint_detailed.c file provided
 * 
 * argc: number of system arguments passed in during execution
 * argv: list of system arguements passed in during execution
 * 
*/
int main(int argc, char *argv[]){
    // initialize all the condition variable and thread lock will be used
    gettimeofday(&init_time, NULL);
    init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);  // line 13 of sample_gettimeofday.c
    pthread_t clerkThrdArray[CLERK_COUNT];
    void *clerkRetThrdArray[CLERK_COUNT];
    int i, rc;
    double avg_customer_wait, avg_bus_wait, avg_econ_wait;
    char *filename = NULL;

    // get input file, open file, collect the file data
    filename = argv[1];
    FILE *fptr = open_file(filename);
    customer_info = read_customer_info(fptr, &total_customers);

    pthread_t customerThrdArray[total_customers];
    void *customerRetThrdArray[total_customers];

    for(i = 0; i < CLERK_COUNT; i++){
        clerks[i].id = i;
        clerks[i].state = 0;
        if((rc = pthread_create(&clerkThrdArray[i], NULL, &clerk_entry, (void *)&clerks[i]))){
            fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
        }
    }
    
    // create customer threads on customer arrival
    for(i = 0; i < total_customers; i++){
        if((rc = pthread_create(&customerThrdArray[i], NULL, &customer_entry, (void *)&customer_info[i]))){
            fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
        }
    }

    // wait for all customer threads to terminate
    for(i = 0; i < total_customers; i++){
        pthread_join(customerThrdArray[i], &customerRetThrdArray[i]);
    //    printf("Thread %d returned\n", i+1);
    }
    
    // join for clerk threads
    for(i = 0; i < CLERK_COUNT; i++){
        pthread_join(clerkThrdArray[i], &clerkRetThrdArray[i]);
   //     printf("Thread %d returned\n", i+1);
    }


    // clean up mutex and condition variables
    pthread_mutex_destroy(&econQueueMutex);
    pthread_mutex_destroy(&busQueueMutex);
    pthread_mutex_destroy(&customerArrived);
    pthread_mutex_destroy(&customerServed);
    pthread_cond_destroy(&busCustomerQueued);
    pthread_cond_destroy(&econCustomerQueued);

    // calculate the average waiting time for customer service
    avg_customer_wait = get_avg_wait(2);
    avg_bus_wait = get_avg_wait(1);
    avg_econ_wait = get_avg_wait(0);

    // output avg wait times
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n", avg_customer_wait);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n", avg_bus_wait);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n", avg_econ_wait);

    exit(EXIT_SUCCESS);

}