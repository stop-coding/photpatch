#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <pthread.h>

#define lengthof(x) (sizeof(x) / sizeof(x[0]))

static pid_t gettid()
{
	return syscall(SYS_gettid);
}

void new_helloworld(int i)
{
    printf(" i will running always ,id[%d]....\n", i);
    return;
}

void helloworld(int i)
{
    new_helloworld(i);
}

int thread_stop()
{
    return 0;
}

int cmp(const void *a, const void *b)
{
    helloworld(1);
	sleep(1);
    return 0;
}

static void *thread_worker(void* arg) 
{
    int thread_id = *(int *)arg;
    printf("thread id = [ %d ]\n", gettid());
    for(;;) {
        sleep(thread_id%10);
        //printf("thread id is %d, runing...\n", thread_id);
        helloworld(thread_id);
        if (thread_stop()) {
            break;
        }
    }
    printf(" thread[%d] stop and exit\n", thread_id);
}

#define MAX_TEST_THREAD_NUM 3

static void multi_thread(void)
{
    int i;
    int tids[MAX_TEST_THREAD_NUM];
    pthread_t  ids[MAX_TEST_THREAD_NUM] = {0};
    for (i = 0; i < MAX_TEST_THREAD_NUM; i++) {
        tids[i] = i+5;
        if (pthread_create(&ids[i], NULL, thread_worker, &tids[i]) < 0) {
			printf("create thread fail, to free pool.");
		}
    }

    for (i = 0; i < MAX_TEST_THREAD_NUM; i++) {
        pthread_join(ids[i], NULL);
    }
    return;
}


int main(int argc, char **argv)
{
	//int data[] = {1, 2, 3, 4};
	//int needle = 4;

    printf("process id = [ %d ]\n", gettid());
	// ensure we backtrace through dynamically linked symbols too
    //while(1)
	//bsearch(&needle, data, lengthof(data), sizeof(data[0]), cmp);
    multi_thread();
	// not reached
	return 1;
}