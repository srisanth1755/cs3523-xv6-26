#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

extern struct proc proc[NPROC];
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}


uint64
sys_getlevel(void)
{
  return myproc()->current_queue;
}

uint64
sys_getmlfqinfo(void)
{
  int pid;
  uint64 addr;
  struct proc *p;
  struct mlfqinfo info;

  argint(0, &pid);
  argaddr(1, &addr);

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);

    if(p->pid == pid) {

      info.level = p->current_queue;
      info.times_scheduled = p->times_scheduled;
      info.total_syscalls = p->syscount;

      for(int i=0;i<4;i++)
        info.ticks[i] = p->ticks[i];

      release(&p->lock);

      if(copyout(myproc()->pagetable, addr,
                (char *)&info, sizeof(info)) < 0)
        return -1;

      return 0;
    }

    release(&p->lock);
  }

  return -1;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_hello(void)
{
  printf("Hello from the kernel!\n");
  return 0;
}

uint64
sys_getpid2(void)
{
  return myproc()->pid;
}

uint64
sys_getppid(void)
{
  struct proc *p = myproc();

  if (p->parent == 0)
    return -1;

  return p->parent->pid;
}

uint64
sys_getnumchild(void)
{
  struct proc *p;
  struct proc *cur = myproc();
  int count = 0;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->parent == cur && p->state != ZOMBIE){
      count++;
    }
    release(&p->lock);
  }

  return count;
}

uint64
sys_getsyscount(void)
{
  struct proc *p = myproc();
  return p->syscount;
}

uint64
sys_getchildsyscount(void)
{
  int pid;
  struct proc *p;
  struct proc *cur = myproc();
  uint64 count = -1;

  // argint returns void in this xv6 version
  argint(0, &pid);

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);

    if(p->pid == pid &&
       p->parent == cur &&
       p->state != ZOMBIE){
      count = p->syscount;
      release(&p->lock);
      return count;
    }

    release(&p->lock);
  }

  return -1;
}


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
