#include "threadpool.h"
#include "errors.h"
#include <errno.h>

typedef struct tpool_s {
	int thread_ct;

	pthread_mutex_t work_lock; // k

	pthread_mutex_t addr_valid_lock; // m
	pthread_cond_t  addr_valid_signal; // x

	pthread_mutex_t addr_null_lock; // j
	pthread_cond_t  addr_null_signal; // y

	dispatch_fn new_fn; // addr
	void *new_fn_arg;

} tpool_t;


void *wait_for_work(void *tp_v)
{
	tpool_t *tp = tp_v;

	for(;;) {
		pthread_mutex_lock(&tp->addr_valid_lock);
		pthread_mutex_lock(&tp->work_lock);

		while( tp->new_fn == NULL )
			pthread_cond_wait(&tp->addr_valid_signal,
				          &tp->addr_valid_lock);

		dispatch_fn work_func = tp->new_fn;
		void *      work_args = tp->new_fn_arg;


		pthread_mutex_lock(&tp->addr_null_lock);
		tp->new_fn = NULL;
		pthread_mutex_unlock(&tp->addr_null_lock);

		pthread_cond_signal(&tp->addr_null_signal);

		pthread_mutex_unlock(&tp->addr_valid_lock);
		pthread_mutex_unlock(&tp->work_lock);


		work_func(work_args);
	}
}


threadpool create_threadpool(int nth)
{
	if (nth < 1) {
		return NULL;
	}

	tpool_t *tp = malloc(sizeof(*tp));
	if (tp) {
		tp->thread_ct = nth;

		tp->new_fn = NULL;
		tp->new_fn_arg = NULL;

		/* Initialize condition variables */
		pthread_cond_init(&tp->addr_valid_signal);
		pthread_cond_init(&tp->addr_null_signal);

		/* Initialize mutexes */
		pthread_mutex_init(&tp->work_lock,NULL);
		pthread_mutex_init(&tp->addr_valid_lock,NULL);
		pthread_mutex_init(&tp->addr_null_lock,NULL);

		/* spawn required threads */
		int i;
		for( i = 0; i < nth; i++ ) {
			int ret = pthread_create(NULL, NULL,
			                         wait_for_work, tp);

			if (ret != 0) {
				WARN(1,errno,"pthread create %d of %d failed",i,nth);
			}
		}
	}
	return tp;
}

void dispatch(threadpool tp_v, dispatch_fn func, void *func_arg)
{
	tpool_t *tp = tp_v;


	/* Give the pool new work */
	pthread_mutex_lock(&tp->addr_valid_lock);
	tp->new_fn = func;
	tp->new_fn_arg = func_arg;
	pthread_mutex_unlock(&tp->addr_valid_lock);

	pthread_cond_signal(&tp->addr_valid_signal);

	/* wait for work to start */
	pthread_mutex_lock(&tp->addr_null_lock);
	while( &tp->new_fn != NULL )
		pthread_cond_wait(&tp->addr_null_signal,
		                  &tp->addr_null_lock);
	
	/* work started... */
	pthread_mutex_unlock(&tp->addr_null_lock);

	return;
}

void destroy_threadpool(threadpool destroyme)
{
	tpool_t *tp = tp_v;	
}
