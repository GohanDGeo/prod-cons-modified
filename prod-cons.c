/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *  Modified by: Georgios Koutroumpis
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 100000

#define P 8

void *producer (void *args);
void *consumer (void *args);

// The struct we want the queue to accept
typedef struct {
  // The function of the struct
  void * (*work) (void *);
  // The list of items of the struct
  void * args;
  // The start time (of when a product gets in the queue)
  struct timeval start;
  // The start time (of when a product leaves the queue)
  struct timeval end;
} workFunction;

typedef struct {
  // Now the queue accepts workFunction products
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  // Array holding the waiting time for each product for each cons thread count
  int *times;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

// Help variable, so we can terminate the program (for testing)
int consumedProducts;

int main ()
{

  FILE *fp;
  fp = fopen("data.csv", "w");

  // Try for multiple of 2 consumer threads
  for(int Q = 1; Q < 129; Q*=2)
  {
    queue *fifo;
    pthread_t pro, con;
    // Initialize the count
    consumedProducts = 0;

    fifo = queueInit ();
    if (fifo ==  NULL) {
      fprintf (stderr, "main: Queue Init failed.\n");
      exit (1);
    }

    // Initialize the array of holding the times
    fifo->times = (int*) malloc(LOOP * P * sizeof(int));

    // Initialize the arrays holding the threads
    pthread_t producers[P];
    pthread_t consumers[Q];

    //Initialize the prod and con threads
    for(int p = 0; p < P; p++)
      pthread_create (&producers[p], NULL, producer, fifo);
    
    for(int q = 0; q < Q; q++)
      pthread_create (&consumers[q], NULL, consumer, fifo);

    for(int p = 0; p < P; p++)  
      pthread_join (producers[p], NULL);

    // Join the prod and con threads
    for(int q = 0; q < Q; q++)
      pthread_join (consumers[q], NULL);
    
    // Write the avg times to the file
    for (int i=0; i<LOOP*P; i++)
        fprintf(fp, "%d,", fifo->times[i]);
    fprintf(fp, "\n");

    queueDelete (fifo);
  }

  // Close the file
  fclose(fp);
  
  return 0;
}

// Simlpe function that calculates n cosines
//@args:
//args: an array holding all the necessary arguments for the function to run
//NOTE: the first element is the number of angles to calculate, and the rest of the
//elements are the angles.
void work(void *args)
{
  int *a = (int *)args;
  for(int i = 0; i < a[0]; i++)
  {
    double res = cos( (double) a[i+1]);
  }
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {

    //Number of angles
    double n = 10;

    double *args = (double *)malloc((n+1)*sizeof(double));

    args[0] = n;
    for(int j = 1; j < (n+1); j++)
    {
      args[j] = ( rand() % 51 );
    }
    
    workFunction product;
    product.work = &work;
    product.args = args;

    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      //printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    // Get start time
    gettimeofday(&product.start, NULL);
    queueAdd (fifo, product);
    
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction out;

  fifo = (queue *)q;

  while(1){
    pthread_mutex_lock (fifo->mut);


    if( consumedProducts == LOOP*P)
    {
      pthread_mutex_unlock(fifo->mut);
      break;
    }

    consumedProducts++;

    while (fifo->empty) {
      //printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    
    queueDel (fifo, &out);

    // Get end time
    gettimeofday(&out.end, NULL);
    
    // Calculate the
    fifo->times[consumedProducts-1] = (int) ((out.end.tv_sec-out.start.tv_sec)*1e6 + (out.end.tv_usec-out.start.tv_usec));
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    out.work(out.args);

  }

  return (NULL);
}

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

void queueAdd (queue *q, workFunction in)
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

void queueDel (queue *q, workFunction *out)
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
