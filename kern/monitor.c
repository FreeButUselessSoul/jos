// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about the stack", mon_backtrace},
	{ "showmappings", "Display physical page mappings and corresponding permission bits",showmappings},
	{ "setm","Set or clear the permission bits in a particular page",setm},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t *ebp;
	struct Eipdebuginfo info;
	ebp=(uint32_t*)read_ebp();
	cprintf("Stack backtrace:\n");
	while (ebp){
		cprintf("  ebp  %08x  eip  %08x  args  %08x  %08x  %08x  %08x  %08x\n",
		ebp,ebp[1],ebp[2],ebp[3],ebp[4],ebp[5],ebp[6]);
		if(debuginfo_eip(ebp[1],&info)==0){
			cprintf("\t%s:%d: %.*s+%u\r\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, ebp[1] - info.eip_fn_addr);
        }
		else cprintf("\tUnkown position\n");
		ebp = (uint32_t*)*ebp;
	}
	return 0;
}
uintptr_t xtoi(char *buf) {
	uintptr_t res = 0;
	buf+=2;	// skip '0x'
	while (*buf) {
		if (*buf >= 'a' && *buf <= 'f')
			*buf = *buf - 'a' + 10;
		else if (*buf >= 'A' && *buf <= 'F')
			*buf = *buf - 'A' + 10;
		else *buf = *buf - '0';
		res = res * 16 + *buf;
		++buf;
	}
	return res;
}
int stoi(char *buf) {
	int res = 0;
	while (*buf) {
		res = res * 10 + *buf - '0';
		++buf;
	}
	return res;
}
void print_permission(pte_t *pte) {
	if (*pte & PTE_P) cprintf("|PTE_P");
	if (*pte & PTE_W) cprintf("|PTE_W");
	if (*pte & PTE_U) cprintf("|PTE_U");
	cprintf("|\n");
}
extern pte_t *kern_pgdir;
extern pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);
int showmappings(int argc, char **argv, struct Trapframe *tf) {
	if (argc == 1) {
		cprintf("Usage: showmappings 0x<begin_addr> 0x<end_addr>\n");
		return 0;
	}
	uint32_t begin = xtoi(argv[1]), end = xtoi(argv[2]);
	cprintf("begin: %x, end: %x\n", begin, end);
	pde_t *temp_pde;
	physaddr_t temp_phyaddr;
	while (begin < end) {
		temp_pde = kern_pgdir + PDX(begin);
		if (*temp_pde & PTE_PS) {
			if (*temp_pde & PTE_P) {
				cprintf("%x - %x -> ", begin,begin+PTSIZE);
				temp_phyaddr = PTE_ADDR(*temp_pde);
				cprintf("%x - %x  ",temp_phyaddr,temp_phyaddr+PTSIZE);
				print_permission(temp_pde);
			} else cprintf("page not exist: %x\n", begin);
			begin += PTSIZE;
		}
		else {
			pte_t *pte = pgdir_walk(kern_pgdir, (void *) begin, 1);	//create
			if (!pte) panic("boot_map_region panic, out of memory");
			if (*pte & PTE_P) {
				cprintf("%x - %x ->", begin,begin+PGSIZE);
				temp_phyaddr = PTE_ADDR(*pte);
				cprintf("%x - %x  ",temp_phyaddr,temp_phyaddr+PGSIZE);
				print_permission(pte);
			} else cprintf("page not exist: %x\n", begin);
			begin += PGSIZE;
		}
	}
	return 0;
}

int setm(int argc, char **argv, struct Trapframe *tf) {
	if (argc == 1 || argv[3][1]!=0 || argv[2][1]!=0) {
		cprintf("Usage: setm 0xaddr [0|1 :clear or set] [p|w|u]\n");
		return 0;
	}
	uint32_t addr = xtoi(argv[1]);
	pte_t *pte = pgdir_walk(kern_pgdir, (void *)addr, 1);
	cprintf("Page of %x before setm: ", addr);
	print_permission(pte);
	uint32_t perm = 0;
	if (argv[3][0] == 'p') perm = PTE_P;
	if (argv[3][0] == 'w') perm = PTE_W;
	if (argv[3][0] == 'u') perm = PTE_U;
	if (argv[2][0] == '0') 	//clear
		*pte = *pte & ~perm;
	else 	//set
		*pte = *pte | perm;
	cprintf("Page of %x after setm: ", addr);
	print_permission(pte);
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
