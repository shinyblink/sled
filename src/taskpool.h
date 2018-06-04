// Taskpool header
#include "oscore.h"

typedef struct {
	void (*func)(void*);
	void* ctx;
} taskpool_job;

typedef struct {
	int workers;
	int queue_size;

	oscore_mutex lock;

	int mode;

	oscore_task* tasks;
	taskpool_job* jobs;
	int numjobs;
} taskpool; // for now

taskpool* taskpool_create(char* pool_name, int workers, int queue_size);
int taskpool_submit(taskpool* pool, oscore_task task, void* ctx);
void taskpool_wait(taskpool* pool);
void taskpool_destroy(taskpool* pool);
