#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// argument order : %rdi, %rsi, %rdx, %r10, %r8, %r9
	//printf ("system call!\n");
	int syscall_num = f->R.rax;
	
	switch(syscall_num){
		case SYS_HALT:                   /* Halt the operating system. */
			halt();
			break;

		case SYS_EXIT:                   /* Terminate this process. */
			exit(f->R.rdi);
			break;

		case SYS_FORK:                   /* Clone current process. */
			f->R.rax = fork(f->R.rdi, f);
			break;

		case SYS_EXEC:                   /* Switch current process. */
			f->R.rax = exec(f->R.rdi);
			break;

		case SYS_WAIT:                   /* Wait for a child process to die. */
			f->R.rax = wait(f->R.rdi);
			break;

		case SYS_CREATE:                 /* Create a file. */
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;

		case SYS_REMOVE:                 /* Delete a file. */
			f->R.rax = remove(f->R.rdi);
			break;

		case SYS_OPEN:                   /* Open a file. */
			f->R.rax = open(f->R.rdi);
			break;

		case SYS_FILESIZE:               /* Obtain a file's size. */
			f->R.rax = filesize(f->R.rdi);
			break;

		case SYS_READ:                   /* Read from a file. */
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;

		case SYS_WRITE:                  /* Write to a file. */
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;

		case SYS_SEEK:                   /* Change position in a file. */
			seek(f->R.rdi, f->R.rsi);
			break;

		case SYS_TELL:                   /* Report current position in a file. */
			f->R.rax = tell(f->R.rdi);
			break;

		case SYS_CLOSE:                  /* Close a file. */
			close(f->R.rdi);
			break;

		case SYS_DUP2:                   /* Duplicate the file descriptor */
			f->R.rax = dup2(f->R.rdi, f->R.rsi);
			break;

		default:
			break;
	}
}

void
check_address(uint64_t *ptr){
	if(  ptr == NULL 				/* 1. Invalid pointer */
	  || is_kernel_vaddr(ptr)		/* 2. PTR point kernel virual memory */
	  || ! pml4_get_page(thread_current()->pml4, ptr))
	  								/* 3. PTR is unmapped */
		exit(-1);
}

void 
halt (void){
	power_off();
}

void 
exit (int status){
	thread_current()->exit_status = status;
	thread_exit();

}
tid_t fork (const char *thread_name, struct intr_frame *f){
	return process_fork(thread_name, f);
}

/*
int exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

int dup2(int oldfd, int newfd);
*/