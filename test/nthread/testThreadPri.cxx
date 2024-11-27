#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <unistd.h>

#include <iostream>

#include <nthread.h>

using namespace std;

void * thread(void *arg)
{
	sleep(10);
	printf("T self: %lu\n", pthread_self());
	printf("T tid: %u\n", gettid());
	printf("T pid: %u\n", getpid());
	sleep(10);
	return 0;
}


int main()
{
    const void* ptr;

    pthread_t tid;

    printf("Current proccess: %u\n", getpid());

    Nthread th( thread, NULL, "worker" );

    cout << "N.B. this test will fail for non-realtime scheduling models." << endl;

    cout << "our thread is " << th.getTid() << " priority: " << th.getSchedulingPriority() << endl;
    th.setSchedulingPriority( 5 );
    cout << "our thread is " << th.getTid() << " priority: " << th.getSchedulingPriority() << endl;

    th.getReturnValue();

    return 0;
}

