/********************************************************
 * Thread												*
 *														*
 * Handles the execution of threads.					*
 *														*
 * Created by: Travis Sweetser, Nick Filipi				*
 *														*
 ********************************************************/
#include <stdio.h>
#include "thread.h"

#define THREAD_FRAME_SIZE	1024		// The size of the thread's personal stack
#define CONTEXT_SIZE		19			// The size of the thread's context
#define MAX_QUEUE_SIZE		20			// The max number of threads that can be in the queue
#define MAX                 50000       // Delay for mythread() after printing message

// disable an interrupt
#define DISABLE_INTERRUPTS() {  \
    asm("wrctl status, zero");  \
}

// enable an interrupt
#define ENABLE_INTERRUPTS() {   \
    asm("movi et, 1");          \
    asm("wrctl status, et");    \
}

// This will point to the thread that was executing when the interrupt occurred
static tcb *this_thread = NULL;

/*
 * Pointer to the context of the main process.
 * Used to return to the main when all threads are done.
 */
static int *main_context = NULL;


/*
 * Queue is a circular buffer of tcb pointers.  Keep track of front and
 *    number of items in the queue.
 */
typedef struct queue_type
{
    tcb*			 threads[MAX_QUEUE_SIZE];	// Array of pointers
    unsigned int	 front;						// The first-in-line node
    unsigned int     size;						// The number of nodes in the queue
} Queue;


/**
 * Initialize queue with a null pointer
 */
static Queue queue = {NULL, 0, 0};

/**
 * Determines if queue is empty or not.
 */
unsigned int isQueueEmpty(){
	return (queue.size <= 0);
}

/**
 * Determines if queue is full or not.
 */
unsigned int isQueueFull(){
	return (queue.size >= MAX_QUEUE_SIZE);
}

/*
 * Used for debugging purposes only
 */
void queuePrint()
{
	int back = ((queue.front + queue.size)%MAX_QUEUE_SIZE);

	int i = queue.front;
	printf( "[");
	if (i < back){
		while (i<back){
			printf("%d, ", queue.threads[i]->thread_id);
			i=(i+1);
		}
	}
	else if (i>=back && queue.size != 0)
	{
		while (i< MAX_QUEUE_SIZE){
			printf("%d, ", queue.threads[i]->thread_id);
			i=(i+1);
		}
		i=0;
		while (i<back){
			printf("%d, ", queue.threads[i]->thread_id);
			i=(i+1);
		}
	}
	printf("]\n");
}

/**
 * mythread_create creates a thread controller buffer.
 *
 * @param thread_id - the thread id
 * @param (*mythread)(thread_id) - function pointer to mythread in main.  Passes thread_id as parameter.
 *
 * @return tcb*	- pointer to the thread control buffer that was created.
 */
tcb *mythread_create(unsigned int thread_id, void (*mythread)(unsigned int thread_id))
{
	// Pointer to the thread's context
	unsigned int *threadContext;

    // allocate a tcb for a thread
    tcb *newThread;
    newThread = (tcb *)malloc(sizeof(tcb));
    if (newThread == NULL)
    {
        printf("Error allocating space for new tcb!\n");
        exit(1);
    }
    
    // Pointer to thread's TCB
    newThread->thread_id = thread_id;

    // allocate a stack for the new thread
    newThread->stack = (unsigned int *)malloc(sizeof(unsigned int) * THREAD_FRAME_SIZE);
    if (newThread->stack == NULL)
    {
        printf("Error allocating space for tcb's frame!\n");
        exit(1);
    }

    /*
     * Set the context pointer to point to the context section in the frame.
     */
    newThread->context = (unsigned int *)(newThread->stack + THREAD_FRAME_SIZE - CONTEXT_SIZE);
    
    /***
     * Directly change the thread's context in memory.  The registers that are important are:
     *
     *    ea - 18(sp) - the exception return address register
     *    r5 - 17(sp) - contents of r5 are copied to estatus
     *    r4 -  5(sp) - thread parameter
     *    ra -  0(sp) - the return address register
     *    fp - -1(sp) - the frame pointer register
     ***/

    /*
     * When the interrupt occurs, the thread will return to
     *    the mythread function in main.
     */
    newThread->context[18] = (unsigned int)mythread;

    /*
     * Set Processor Interrupt-Enable bit (PIE) = 1 in the r5 register.
     * When PIE=1, internal and maskable external interrupts can be taken.
     */
    newThread->context[17] = 1;

    /*
     * Pass the thread_id as an argument using r4 register.
     */
    newThread->context[5] = thread_id;

    /*
     * Set the return address to the thread_destroy function where
     *   the memory will be released.
     */
    newThread->context[0] = (unsigned int)mythread_destroy;

    /*
     * Store the value of the Frame pointer pointer into memory
     * Will be loaded in the context_switch.
     */
    newThread->context[-1] = (unsigned int *)(newThread->context + CONTEXT_SIZE);

    return newThread;
}

/**
 * Adds the thread to the run queue
 *
 * @param the pointer to the thread to be added to the run queue
 */
void mythread_join(tcb *newThread)
{
    int back;
    if (isQueueFull()){
    	fprintf(stderr, "Unable to add pointer to queue: Queue Full!\n");
    	exit(1);
    }
    
    back = (queue.front + queue.size) % MAX_QUEUE_SIZE;
    queue.threads[back] = newThread;
    
    queue.size++;
}

/**
 * Removes the first-in-line thread from the queue
 *
 * @returns pointer to thread control buffer that was just removed
 *            from the queue.
 */
tcb *mythread_disjoin()
{
    void *newThread = NULL;
    
    if(isQueueEmpty()){
    	fprintf(stderr,"Unable to remove pointer from queue: Queue Empty!\n");
    	exit(1);
    }
    
    newThread = queue.threads[queue.front];
    
    queue.front = (queue.front+1) % MAX_QUEUE_SIZE;
    queue.size--;
    
    return newThread;
}

/**
 *  This is the thread's function.  Do not update this!  
 *  Provided in assignment.
 */
void mythread(int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	int i, j, n;
	n = (thread_id % 2 == 0)? 10: 15;
	for (i = 0; i < n; i++){
	    printf("This is message %d of thread #%d.\n", i, thread_id);
	    for (j = 0; j < MAX; j++);
	}
}

/**
 * Schedules the threads in the run queue.
 * 	Will pick the thread at the front of the queue and remove it from the queue.
 * 	When the thread is interrupted the thread's context pointer will be added to
 *
 *
 * @param the pointer to the context of the thread that was executing before the interrupt
 *
 * @return the pointer to the context of the thread that will now be running
 */
void *myscheduler(void *context)
{

	// Only switch the context after the main has created and joined threads.
	if (!isQueueEmpty())
	{
	    // If the main_context is null, then this is the main's context
	    if (main_context == NULL)
	    {
	        // Save the context of the main
	    	main_context = (unsigned int *)context;
	    }
		// This thread hasn't completed its execution, so add it to the end of the line
	    else if (this_thread != NULL)
        {
        	// Set pointer to the new thread's context
        	this_thread->context = (unsigned int *)context;

        	// Add the thread to the run queue.
        	mythread_join(this_thread);
        }

    	queuePrint();

        /*
         * Set context pointer to point to the context of the first-in-line thread
         *   and remove the thread from the wait queue.
         */
        this_thread = mythread_disjoin(this_thread);
        context = (void *)(this_thread->context);
    }
	// The only thread left is the main, so return to the main.
    else if (this_thread==NULL && main_context!=NULL)
    {        
    	// Proceed with the execution of the main process
    	alt_printf("Interrupted by the DE2 timer!\n");
        context = (void *)main_context;
    }

	resetFlag();
    return context;
}

/**
 * Releases the memory that was used by the thread and sets the thread to null.
 *
 * Interrupts must be disabled while this is happening to ensure that the thread is completely
 * destroyed before another thread begins execution.
 */
void mythread_destroy()
{
    DISABLE_INTERRUPTS();
    free(this_thread->stack);
    free(this_thread);
    this_thread = NULL;
    ENABLE_INTERRUPTS();
    while(1);
}
