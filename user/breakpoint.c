// program to cause a breakpoint trap and then continue
// Custom File by Chengxuan Zhu 1900012915

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $3");
	cprintf("Passing Breakpoint 1...\n");
	asm volatile("int $3");
	cprintf("Passing Breakpoint 2...\n");
	cprintf("Stepping 1\n");
	cprintf("Stepping 2\n");
}

