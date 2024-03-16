#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "thread_pool.h"

static void *thread_worker(void *args);

bool init_thread_pool(Pool *pool, int n_threads)
{
	int error, i;

	pool->head = NULL;
	pool->tail = NULL;
	pool->pause = false;
	pool->n_threads = n_threads;
	pthread_mutex_init(&pool->lock, NULL);
	pthread_cond_init(&pool->notify, NULL);

	pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * n_threads);
	for (i = 0; i < n_threads; i++)
	{
		error = pthread_create(&pool->threads[i], NULL, thread_worker, pool);
		if (error != 0)
		{
			printf("Failed to create thread: %s\n", strerror(error));
			return false;
		}
	}

	return true;
}

bool thread_pool_add_job(Pool *pool, void (*function)(void *), void *args)
{
	Job *job = (Job *)malloc(sizeof(Job));

	if (job == NULL)
	{
		return false;
	}

	job->func = function;
	job->args = args;
	job->next = NULL;

	pthread_mutex_lock(&pool->lock);
	if (pool->head == NULL)
	{
		pool->head = pool->tail = job;
	}
	else
	{
		pool->tail->next = job;
		pool->tail = job;
	}

	pthread_mutex_unlock(&pool->lock);
	pthread_cond_signal(&pool->notify);

	return true;
}

void thread_pool_destroy(Pool *pool)
{
	pthread_mutex_lock(&pool->lock);
	pool->pause = true;
	pthread_mutex_unlock(&pool->lock);

	pthread_cond_broadcast(&pool->notify);

	int i;
	for (i = 0; i < pool->n_threads; i++)
	{
		pthread_join(pool->threads[i], NULL);
	}

	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->notify);
}

void *thread_worker(void *args)
{
	Pool *pool = (Pool *)args;

	while (1)
	{
		pthread_mutex_lock(&pool->lock);
		while (pool->head == NULL && !pool->pause)
		{
			pthread_cond_wait(&pool->notify, &pool->lock);
		}

		if (pool->pause)
		{
			break;
		}

		Job *job = pool->head;
		pool->head = job->next;
		pthread_mutex_unlock(&pool->lock);

		(*(job->func))(job->args);
		free(job);
	}

	return NULL;
}
