/*
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		      pthreads.
 *
 *	Author	: Andrae Muys - 18 SEPT 1997
 *
 *	Revised	: Angelos Spyrakis - 31 MAR 2021
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 20000 // The load of each producer thread

//! Global variables
int p, q; // p=producer threads, q=consumer threads
int consumed = 0; // consumed in the end will be p*LOOP
double suspension_time;

//! PROD-CONS Functions
void *producer (void *args);
void *consumer (void *args);

// A function simulating a workload for a thread
void *work_function();
// To calculate the difference between two timespecs
double elapsed_time(struct timespec start, struct timespec end);

typedef struct{
	void *(*work)(void *);
    void *arg;
    struct timespec start; //to count the suspension time
} workFunction;

typedef struct{
    workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(void);
void queueDelete(queue *qu);
void queueAdd(queue *qu, workFunction in);
void queueDel(queue *qu, workFunction *out);

int main (int argc, char** argv)
{
    char* s1 = argv[1];
    char* s2 = argv[2];
    p = atoi(s1);
    q = atoi(s2);

    queue *fifo;
    pthread_t *pro, *con;
    pro = (pthread_t *)malloc(p*sizeof(pthread_t));
    if(pro==NULL){
        exit(1);
    }
    con = (pthread_t *)malloc(q*sizeof(pthread_t));
    if(con==NULL){
        exit(1);
    }

    fifo = queueInit ();
    if (fifo ==  NULL){
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }
    //! Create the threads
    int status;
    for(long i=0; i<p; i++){
        status = pthread_create(&pro[i], NULL, producer, fifo);
        if (status){
            exit(2);
        }
    }
    for(long i=0; i<q; i++){
        status = pthread_create(&con[i], NULL, consumer, fifo);
        if (status){
            exit(2);
        }
    }
    for (int i=0; i<p; i++){
        pthread_join(pro[i], NULL);
    }
    for (int i=0; i<q; i++){
        pthread_join(con[i], NULL);
    }

    queueDelete(fifo);
    double suspended_avg = (suspension_time / (double)(LOOP*(double)p))*(double)1000000;
    printf("Producers = %d | Consumers = %d | Avg. Time Suspended: %fus\n", p, q, suspended_avg);
    pthread_exit(NULL);
    return 0;
}

void *producer (void *qu)
{
    queue *fifo;
    fifo = (queue *)qu;
    workFunction fun;
    fun.work = work_function;

    for (int i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->full){
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        clock_gettime(CLOCK_MONOTONIC, &fun.start);

        queueAdd(fifo, fun);
		pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }
    return(NULL);
}

void *consumer (void *qu)
{
    queue *fifo;
    fifo = (queue *)qu;
    workFunction fun;

    while(1){
        pthread_mutex_lock(fifo->mut);
        while(fifo->empty) {
            if(consumed == (p*LOOP)){
				pthread_mutex_unlock (fifo->mut);
                pthread_cond_signal (fifo->notEmpty);
                return (NULL);
            }
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);

        queueDel (fifo, &fun);
        consumed++;
        suspension_time += elapsed_time(fun.start, end);
        fun.work;
		pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
        
    }
    return(NULL);
}

queue *queueInit (void){
    queue *qu;

    qu = (queue *)malloc (sizeof (queue));
    if (qu == NULL) return (NULL);
    qu->empty = 1;
    qu->full = 0;
    qu->head = 0;
    qu->tail = 0;
    qu->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (qu->mut, NULL);
    qu->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (qu->notFull, NULL);
    qu->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (qu->notEmpty, NULL);
    return (qu);
}

void queueDelete (queue *qu){
    pthread_mutex_destroy (qu->mut);
    free (qu->mut);
    pthread_cond_destroy (qu->notFull);
    free (qu->notFull);
    pthread_cond_destroy (qu->notEmpty);
    free (qu->notEmpty);
    free (qu);
}

void queueAdd (queue *qu, workFunction in){
    qu->buf[qu->tail] = in;
    qu->tail++;
    if (qu->tail == QUEUESIZE)
        qu->tail = 0;
    if (qu->tail == qu->head)
        qu->full = 1;
    qu->empty = 0;

    return;
}

void queueDel (queue *qu, workFunction *out){
    *out = qu->buf[qu->head];

    qu->head++;
    if (qu->head == QUEUESIZE)
        qu->head = 0;
    if (qu->head == qu->tail)
        qu->empty = 1;
    qu->full = 0;

  return;
}

void *work_function(){
    time_t t;
    srand((unsigned) time(&t));
    for(int i=1; i<11; i++){
        double val = cos((double)(rand()%20));
    }
}

double elapsed_time(struct timespec start, struct timespec end){
    struct timespec temp;
    if((end.tv_nsec-start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else{
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    double return_val = (double)temp.tv_sec + (double)((double)temp.tv_nsec / (double)1000000000);
    return return_val;
}

