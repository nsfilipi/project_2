/********************************************************
 * Thread												*
 *														*
 * Manages all of the threads.							*
 *														*
 * Created by: Travis Sweetser, Nick Filipi				*
 *														*
 ********************************************************/

/* thread control block */
typedef struct
{
    unsigned int thread_id;					// ID of the thread
    unsigned int *stack;					// Pointer to the bottom of the thread's stack
    unsigned int *context;					// Pointer to the bottom of the thread's context section in the thread's stack
} tcb;


/*
 * Create a new thread and return a pointer to newly created thread
 */
tcb *mythread_create(unsigned int thread_id, void (*mythread)(unsigned int thread_id));

/*
 * Handles execution of the threads
 */
void *myscheduler(void *context);

/*
 * Add the thread to the run queue
 */
void mythread_join(tcb *newThread);

/*
 * Release the memory that is occupied by the thread
 */
void mythread_destroy();

/*
 * The thread's function.
 */
void mythread();

