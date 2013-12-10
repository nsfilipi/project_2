#include <stdio.h>
#include <alt_types.h>
#include <sys/alt_alarm.h>

#include "thread.h"

#define NUM_THREADS     8                                   // The number of threads to create
#define MAX             50000                               // Delay for os_prototype after printing message
#define QUANTUM_LENGTH  10                                  // Amount of time before interrupt occurs
#define ALARMTICKS(x)   ((alt_ticks_per_second()*(x))/10)   // Formula to create timer delay

// An alarm
static alt_alarm timer;

// Global flag for alarm interrupt
unsigned int global_flag = 0;

// Determine if flag is set
unsigned int isFlagSet()
{
    return global_flag != 0;
}

// Reset the global flag
void resetFlag()
{
    global_flag = 0;
}

// Alarm interrupt handler
alt_u32 myinterrupt_handler(void* context)
{
    global_flag = 1;
    return alt_ticks_per_second();
}

/*
 *  Creates threads, joins threads, then endlessly prints message.
 */
void os_prototype()
{
    unsigned int i;
    tcb *newThread;

    for (i = 0; i < NUM_THREADS; i++)
    {
        // Create a thread with id=i that will execute mythread
        newThread = mythread_create(i, mythread);
        // Add the thread to the wait queue
        mythread_join(newThread);
    }
    
    if (alt_alarm_start(&timer, ALARMTICKS(QUANTUM_LENGTH), myinterrupt_handler, NULL) < 0)
    {
        printf("No system clock available\r\n");
    }

    /* an endless while loop */
    while (1)
    {
        printf ("This is the OS prototype for my exciting CSE351 course projects!\n");
        
        /* delay printf for a while */
        for (i = 0; i < MAX; i++);
    }
}

typedef struct
{
	char* sem_type; //This is only used to display what semaphore is blocking a thread
	int sem_count; //This is the semaphore value
	Q_type* queue; //This queue contains the tcb's of the threads blocked by this semaphore
} Semaphore;

static Semaphore* full;
static Semaphore* empty;
static Semaphore* mutex;

Semaphore* mysem_create(int count, char* sem_type){
	Semaphore* sem = (Semaphore*) malloc(sizeof(Semaphore));
	sem->sem_count = count;
	sem->sem_type = sem_type;
	sem->queue = (Q_type*) malloc(sizeof(Q_type));
	sem->queue->head = NULL;
	sem->queue->tail = NULL;
	sem->queue->size = 0;
	return sem;
}

void mysem_signal(Semaphore* sem){
	DISABLE_INTERRUPTS();
	sem->sem_count++;
	if(sem->sem_count <= 0){
	mythread_unblock(dequeueSem(sem->queue));
	}
	ENABLE_INTERRUPTS();
}

void mysem_wait(Semaphore* sem){
	DISABLE_INTERRUPTS();
	sem->sem_count--;
if(sem->sem_count < 0){
	tcb* currentThread = getCurrentThread();
	mythread_block(currentThread, sem->sem_type);
	enqueueSem(sem->queue, currentThread);
	//block process and allow interrupts
	ENABLE_INTERRUPTS();
	while(currentThread->state == BLOCKED){
	asm("nop");
	}
	} else {
	ENABLE_INTERRUPTS();
	}
}

void mysem_delete(Semaphore* sem){
	free(sem);
}

void mythread(int thread_id)
{
	int n, i, j;
	if(thread_id < 4){
	n = 10;
	} else {
	n = 15;
	}
	if(thread_id % 2 == 0){ //Consumer
	for (i = 0; i < n; i++){
	mysem_wait(full);
	mysem_wait(mutex);
	buffer_count--;
	buffer[buffer_count] = ' ';
	printf("Thread %d consumed item #%d. Buffer count = %d\n", thread_id, i, buffer_count);
	mysem_signal(mutex);
	mysem_signal(empty);
	for (j = 0; j < MAX/2; j++);
	}
	} else { //Producer
	for (i = 0; i < n; i++){
	mysem_wait(empty);
	mysem_wait(mutex);
	buffer[buffer_count] = 'X';
	buffer_count++;
	printf("Thread %d produced item #%d. Buffer count = %d\n", thread_id, i, buffer_count);
	mysem_signal(mutex);
	mysem_signal(full);
	for (j = 0; j < MAX/2; j++);
	}
	}
}

/*
 *  Begins OS prototype
 */
int main()
{
    os_prototype();
    return 0;
}
