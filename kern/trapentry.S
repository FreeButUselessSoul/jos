/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)	\
.data;		\
	.long name;	\
.text;					\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */

#define TRAPHANDLER_NOEC(name, num)					\
.data;			\
	.long name;	\
.text;			\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

#define placeholder()\
.data;\
	.long 0

.data
	.p2align 2
	.globl funs
funs:
.text
/*
 * Challenge: my code here
 */
	TRAPHANDLER_NOEC(th0, 0)
	TRAPHANDLER_NOEC(th1, 1)
	placeholder()
	TRAPHANDLER_NOEC(th3, 3)
	TRAPHANDLER_NOEC(th4, 4)
	TRAPHANDLER_NOEC(th5, 5)
	TRAPHANDLER_NOEC(th6, 6)
	TRAPHANDLER_NOEC(th7, 7)
	TRAPHANDLER(th8, 8)
	TRAPHANDLER_NOEC(th9, 9)
	TRAPHANDLER(th10, 10)
	TRAPHANDLER(th11, 11)
	TRAPHANDLER(th12, 12)
	TRAPHANDLER(th13, 13)
	TRAPHANDLER(th14, 14)
	placeholder()
	TRAPHANDLER_NOEC(th16, 16)
	TRAPHANDLER_NOEC(thsyscall,T_SYSCALL)
/**
 * Lab 4: Handlers for interrupts
 */
	TRAPHANDLER_NOEC(handler_timer, IRQ_OFFSET + IRQ_TIMER)
	TRAPHANDLER_NOEC(handler_kbd, IRQ_OFFSET + IRQ_KBD)
	TRAPHANDLER_NOEC(handler_serial, IRQ_OFFSET + IRQ_SERIAL)
	TRAPHANDLER_NOEC(handler_spurious, IRQ_OFFSET + IRQ_SPURIOUS)
	TRAPHANDLER_NOEC(handler_ide, IRQ_OFFSET + IRQ_IDE)
	TRAPHANDLER_NOEC(handler_error, IRQ_OFFSET + IRQ_ERROR)
/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	pushl %ds
	pushl %es
	pushal
	pushl $GD_KD
	popl %ds
	pushl $GD_KD
	popl %es
	pushl %esp
	call trap