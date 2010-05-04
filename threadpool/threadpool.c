#include <pthread.h>
#include <stdlib.h>

#include "threadpool.h"
#include "errors.h"

typedef struct tpool_s {
	int thread_ct;

	//pthread_mutex_t work_lock; // k

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
		
		/* aquire M  & K */
		//pthread_mutex_lock(&tp->work_lock);
		//INFO("aquired work_lock");
		pthread_mutex_lock(&tp->addr_valid_lock);
		INFO("aquired addr_valid_lock");
		

		while( tp->new_fn == NULL ) {
			pthread_cond_wait(&tp->addr_valid_signal,
				          &tp->addr_valid_lock);
		}

		dispatch_fn work_func = tp->new_fn;
		void *      work_args = tp->new_fn_arg;

		pthread_mutex_lock(&tp->addr_null_lock);
		tp->new_fn = NULL;
		pthread_cond_signal(&tp->addr_null_signal);
		pthread_mutex_unlock(&tp->addr_null_lock);

		pthread_mutex_unlock(&tp->addr_valid_lock);
		//pthread_mutex_unlock(&tp->work_lock);

		work_func(work_args);
	}
}

void dispatch(threadpool tp_v, dispatch_fn func, void *func_arg)
{
	tpool_t *tp = tp_v;


	/* Give the pool new work */
	pthread_mutex_lock(&tp->addr_valid_lock);
	tp->new_fn = func;
	tp->new_fn_arg = func_arg;
	pthread_cond_signal(&tp->addr_valid_signal);
	pthread_mutex_unlock(&tp->addr_valid_lock);


	/* wait for work to start */
	pthread_mutex_lock(&tp->addr_null_lock);
	while( tp->new_fn != NULL ) {
		pthread_cond_wait(&tp->addr_null_signal,
		                  &tp->addr_null_lock);
	}
	
	/* work started... */
	pthread_mutex_unlock(&tp->addr_null_lock);

	return;
}

void destroy_threadpool(threadpool tp_v)
{
	tpool_t *tp = tp_v;
	
	{
		int i;
		for( i = 0; i < tp->thread_ct; i++ ) {
			dispatch(tp_v,pthread_exit,NULL);
		}
	}

	int ret;
	ret = pthread_mutex_destroy(&tp->addr_valid_lock);
	if (ret != 0) {
		WARN(1,ret,"pthread mutex destroy of addr_valid_lock failed");
	}
	ret = pthread_mutex_destroy(&tp->addr_null_lock);
	if (ret != 0) {
		WARN(1,ret,"pthread mutex destroy of addr_null_lock failed");
	}
	//ret = pthread_mutex_destroy(&tp->work_lock);
	//if (ret != 0) {
	//	WARN(1,ret,"pthread mutex destroy of work_lock failed");
	//}


	ret = pthread_cond_destroy(&tp->addr_valid_signal);
	if (ret != 0) {
		WARN(1,ret,"pthread cond destroy of addr_valid_signal failed");
	}
	
	ret = pthread_cond_destroy(&tp->addr_null_signal);
	if (ret != 0) {
		WARN(1,ret,"pthread cond destroy of addr_null_signal failed");
	}

	free(tp);
}


threadpool create_threadpool(int nth)
{

	if (nth < 1) {
		return NULL;
	}

	tpool_t *tp = malloc(sizeof(*tp));
	if (!tp) {
		return NULL;
	}

	tp->thread_ct = nth;

	tp->new_fn = NULL;
	tp->new_fn_arg = NULL;

	/* Initialize condition variables */
	int ret = pthread_cond_init(&tp->addr_valid_signal,NULL);
	if (ret != 0) {
		WARN(1,ret,"pthread cond init of addr_valid_signal failed");
	}


	ret = pthread_cond_init(&tp->addr_null_signal,NULL);
	if (ret != 0) {
		WARN(1,ret,"pthread cond init of addr_null_signal failed");
	}


	/* Initialize mutexes */
	//pthread_mutex_init(&tp->work_lock,NULL);
	ret = pthread_mutex_init(&tp->addr_valid_lock,NULL);
	if (ret != 0) {
		WARN(1,ret,"pthread cond init of addr_valid_lock failed");
	}

	ret = pthread_mutex_init(&tp->addr_null_lock,NULL);
	if (ret != 0) {
		WARN(1,ret,"pthread cond init of addr_null_lock failed");
	}

	/* spawn required threads */
	int i;
	for( i = 0; i < nth; i++ ) {
		pthread_t thid;
		int ret = pthread_create(&thid, NULL,
		                         wait_for_work, tp);


		pthread_detach(thid);
		if (ret != 0) {
			WARN(1,ret,"pthread create %d of %d failed",i,nth);
		}
	}
	return tp;
}

