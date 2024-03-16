#include <stdbool.h>
#include <pthread.h>

typedef struct Job
{
	void (*func)(void *);
	void *args;
	struct Job *next;
} Job;

typedef struct
{
	Job *head;
	Job *tail;
	bool pause;
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t *threads;
	unsigned int n_threads;
} Pool;

bool init_thread_pool(Pool *pool, int n_threads);
void thread_pool_destroy(Pool *pool);
bool thread_pool_add_job(Pool *pool, void (*function)(void *), void *arg);
