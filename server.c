#include "server.h"
#include "tiny.h"

//pthread_barrier_t mybarrier;
int main (int argc, char **argv)
{
	int i;
	queue *fifo;
	pthread_t pro, con[4];
	int port;
	struct sched_param my_param;
	pthread_attr_t hp_attr;
	pthread_attr_t lp_attr;
	int min_priority, policy;
	my_param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	min_priority = my_param.sched_priority;
	pthread_setschedparam(pthread_self(), SCHED_RR, &my_param);
	pthread_getschedparam (pthread_self(), &policy, &my_param);


        fifo = queueInit ();
	if (fifo ==  NULL) {
		fprintf (stderr, "main: Queue Init failed.\n");
		exit (1);
	}
	if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
        }
        port = atoi(argv[1]);
	listenfd = Open_listenfd(port);

      /* SCHEDULING POLICY AND PRIORITY FOR OTHER THREADS */
      pthread_attr_init(&lp_attr);
      //pthread_attr_init(&mp_attr);
      pthread_attr_init(&hp_attr);

      pthread_attr_setinheritsched(&lp_attr, PTHREAD_EXPLICIT_SCHED);
      //pthread_attr_setinheritsched(&mp_attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setinheritsched(&hp_attr, PTHREAD_EXPLICIT_SCHED);

      pthread_attr_setschedpolicy(&lp_attr, SCHED_FIFO);
      //pthread_attr_setschedpolicy(&mp_attr, SCHED_FIFO);
      pthread_attr_setschedpolicy(&hp_attr, SCHED_FIFO);

      my_param.sched_priority = min_priority + 1;
      pthread_attr_setschedparam(&lp_attr, &my_param);
      //my_param.sched_priority = min_priority + 2;
      //pthread_attr_setschedparam(&mp_attr, &my_param);
      my_param.sched_priority = min_priority + 3;
      pthread_attr_setschedparam(&hp_attr, &my_param);
      //pthread_barrier_init(&mybarrier,NULL, PRIO_GROUP*3);

	
       //Create Producer thread
       pthread_create (&pro, &hp_attr, producer, fifo);
       //Create Worker threads	
       for(i=0;i<4;i++)	
        pthread_create (&con[i],&lp_attr, consumer, fifo);
        
        pthread_join (pro, NULL);
	for(i=0;i<4;i++)
	pthread_join (con[i], NULL);
	queueDelete (fifo);
	
        //pthread_barrier_destroy(&mybarrier);
	pthread_attr_destroy(&lp_attr);
	pthread_attr_destroy(&hp_attr);
	return 0;
}

void *producer (void *q)
{
	queue *fifo;
	int *connfd,clientlen;
    	struct sockaddr_in clientaddr;	
	int i;
        pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;

	fifo = (queue *)q;
	
        ret = pthread_getschedparam (thread_id, &policy, &param);
        priority = param.sched_priority;	
        printf("priority of producer: %d \n", priority);
	
	for(i=0;i<LOOP;i++) {
	
		pthread_mutex_lock (fifo->mut);
		while (fifo->full) {
			printf ("producer: queue FULL.\n");
			pthread_cond_wait (fifo->notFull, fifo->mut);
		}
 
        clientlen = sizeof(clientaddr);
	connfd=Malloc(sizeof(int));
	*connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        queueAdd (fifo, *connfd);

		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notEmpty);
		millisleep (200);
	}
	return (NULL);
}


void *consumer (void *q)
{
	queue *fifo;
	int fd;
	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;

	fifo = (queue *)q;
	ret = pthread_getschedparam (thread_id, &policy, &param);
	priority = param.sched_priority;	
	printf("priority of consumer: %d \n", priority);

	        while(1){
		pthread_mutex_lock (fifo->mut);
		while (fifo->empty) {
			printf ("consumer: queue EMPTY.\n");
		pthread_cond_wait (fifo->notEmpty, fifo->mut);
		}
		queueDel (fifo, &fd);
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notFull);
		printf ("consumer: recieved %d.\n", fd);
		doit(fd); 		
millisleep(100);
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

void queueAdd (queue *q, int in)
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

void queueDel (queue *q, int *out)
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

void millisleep(int milliseconds)
{
      usleep(milliseconds * 1000);
}










