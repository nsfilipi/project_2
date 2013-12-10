/********************************************************
 * Context Switch										*
 *														*
 * When this code is injected the current value of the 	*
 *  sp is the pointer to the context section of 		*
 *  the thread that was interrupted.					*
 *														*
 * Just before switching the context pointer, save the 	*
 *  frame pointer. After switching, load the new frame	*
 *  frame pointer.										*
 *														*
 * Created by: Travis Sweetser, Nick Filipi				*
 *														*
 ********************************************************/


.section .exceptions.exit.user, "xa"
// only switch contexts if global_flag = 1 (interrupt triggered by alarm)
call isFlagSet
beq  r2, r0, end_switch
// r4 is the argument register.  Pass sp to myscheduler.
add  r4, sp, r0
call myscheduler
// Before switching contexts, store the current thread's frame pointer.
stw  fp, -4(sp)
// r2 contains the return value from myscheduler. Set sp to new thread's context.
add  sp, r2, r0
// Load the new thread's frame pointer.
ldw  fp, -4(sp)
end_switch:

