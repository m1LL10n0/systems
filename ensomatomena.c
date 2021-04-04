#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>


#define THREADSNUM 4
#define QUEUESIZE 10
#define LOOP 1000000

void *producer (void *args);
void *consumer (void *args);

float totalTime = 0;
int itemsLeft = LOOP;

pthread_mutex_t mutexsum;

struct workFunction
{
    void * (*work)(void *);
    void * arg;
    time_t seconds;
    suseconds_t useconds;
};

typedef struct
{
    struct workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction in);
void queueDel (queue *q, struct workFunction *out);
void* randomFunction(void *ptr);

int main ()
{
    queue *fifo;
    pthread_t threads[THREADSNUM];
    pthread_mutex_init(&mutexsum, NULL);

    fifo = queueInit ();
    if (fifo ==  NULL)
    {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }
    for(int i=0; i<THREADSNUM; i++)
    {
        if(i == 0)
        {
            pthread_create (&threads[i], NULL, producer, fifo);
        }
        else
        {
            pthread_create (&threads[i], NULL, consumer, fifo);
        }
    }
    for(int i=0; i<THREADSNUM; i++)
    {
        pthread_join (threads[i], NULL);
    }
    queueDelete (fifo);
    pthread_mutex_destroy(&mutexsum);
    printf("mean time elapsed is: %f\n", totalTime/LOOP);
    return 0;

}
//###################################################################
void *producer (void *q)
{
    queue *fifo;
    struct timeval current_time;

    struct workFunction creation;

    fifo = (queue *)q;

    for (int i = 0; i < LOOP; i++)
    {
        creation.work = randomFunction;
        creation.arg = (void*)(intptr_t)i;
        pthread_mutex_lock (fifo->mut);
        while (fifo->full)
        {
            //printf ("producer: queue FULL!!!!!!!!!!\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        gettimeofday(&current_time, NULL);
        creation.seconds = current_time.tv_sec;
        creation.useconds = current_time.tv_usec;
        queueAdd (fifo, creation);
        pthread_cond_broadcast(fifo->notEmpty);
        pthread_mutex_unlock (fifo->mut);

        //printf ("producer: produced\n");
        //usleep(10000);
    }
    pthread_exit(NULL);
}
//###################################################################
void *consumer (void *q)
{
    queue *fifo;
    struct timeval current_time;

    struct workFunction received; //received current time of product
    long deltaSeconds;
    long deltaUseconds;
    long timeElapsed;

    fifo = (queue *)q;

    while(1)
    {
        pthread_mutex_lock (fifo->mut);
        if(itemsLeft == 0)
        {
            //printf("consumer[%ld]...exitinG.................\n", pthread_self());
            pthread_mutex_unlock (fifo->mut);
            break;
        }
        while (fifo->empty)
        {
            //printf ("consumer[%ld]: queue EMPTY!!!!!!!!! BLOCKING\n", pthread_self());
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
            //printf ("consumer[%ld]: [[[UN]]]BLOCKING\n", pthread_self());
            if(itemsLeft == 0)
            {
                pthread_mutex_unlock (fifo->mut);
                //printf("consumer[%ld]...exiting(from block).................\n", pthread_self());
                pthread_exit(NULL);
            }
        }
        queueDel (fifo, &received);
        gettimeofday(&current_time, NULL);
        itemsLeft--;
        pthread_cond_signal (fifo->notFull);
        pthread_mutex_unlock (fifo->mut);

        deltaSeconds = current_time.tv_sec - received.seconds;
        deltaUseconds = current_time.tv_usec - received.useconds;
        timeElapsed = 1000000*deltaSeconds + deltaUseconds;

        received.work(received.arg); //run function

        pthread_mutex_lock(&mutexsum);
        totalTime = totalTime + timeElapsed;
        pthread_mutex_unlock(&mutexsum);

        //printf ("consumer[%ld]: received %ld\n",pthread_self(), timeElapsed);
        //usleep(100000);

    }
    pthread_exit(NULL);

}
//###################################################################
/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}

void queueDelete (queue *q)
{
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}

void queueAdd (queue *q, struct workFunction in)
{
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel (queue *q, struct workFunction *out)
{
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

void* randomFunction(void *ptr)
{
    double k = 0;
    for(int i=0; i<3; i++)
    {
        k = k + sin(i) + cos(i);
    }

    // printf("function executed\n");
}
