#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREAD 5

void *PrintHello(void *arg)
{
    int thread_arg;

    thread_arg = *((int *)arg);
    printf("Hello from thread %d\n", thread_arg);
    free(arg);
}

void create_thread(int i)
{
        int *pId = NULL;
	pthread_t tid = 0;
	int ret = 0;

	printf("Creating thread %d\n", i);
	pId = (int *)malloc(sizeof(int));
	memset(pId, 0, sizeof(int));
	memcpy(pId, &i, sizeof(int));
	printf("pId=%d i=%d\n", *pId, i);
	ret = pthread_create(&tid, NULL, PrintHello, pId);
	if (ret)
	{
	        printf("ERROR, return code is %d\n", ret);
	}
	printf("thread %d created\n\n", tid);
}


int main()
{
    int i;

    for(i = 0; i < NUM_THREAD; i++)
    {
	create_thread(i);
    }
    sleep(3);
    return 0;
}
