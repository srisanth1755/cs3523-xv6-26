#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

// int mlfq_quantum[4] = {2, 4, 8, 16};
// int global_ticks = 0;

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from, and returns to, trampoline.S
// return value is user satp for trampoline.S to switch to.
//
uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);  //DOC: kernelvec

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if((r_scause() == 15 || r_scause() == 13) &&
            vmfault(p->pagetable, r_stval(), (r_scause() == 13)? 1 : 0) != 0) {
    // page fault on lazily-allocated page
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // // give up the CPU if this is a timer interrupt.
  // if(which_dev == 2)
  //   yield();
  if(which_dev == 2) {   // timer interrupt

    struct proc *p = myproc();

    if(p && p->state == RUNNING) {

      //acquire(&p->lock);
      p->ticks_used++;
      p->ticks[p->current_queue]++;
      if(p->ticks_used >= mlfq_quantum[p->current_queue]) {

        uint64 deltaS = p->syscount - p->last_syscall_count;
        int deltaT = p->ticks_used;
        if(deltaS < deltaT) {
          if(p->current_queue < 3)
            p->current_queue++;
        }

        p->ticks_used = 0;

        //release(&p->lock);

        yield();
        
      }

      //release(&p->lock);
    }

    //Global boost counter
    global_ticks++;
    if(global_ticks % 128 == 0)
      priority_boost();
  }
  

    // ===== SC-MLFQ timer handling =====
  // if(which_dev == 2){
  //   struct proc *p = myproc();

  //   if(p){
  //     p->ticks_used++;
  //     p->ticks[p->current_queue]++; 
  //     global_ticks++;

  //     printf("tick pid %d level %d ticks %d\n",
  //          p->pid, p->current_queue, p->ticks_used);

  //     // check time slice expiry
  //     if(p->ticks_used >= mlfq_quantum[p->current_queue]){

  //       int deltaS = p->syscount - p->last_syscall_count;
  //       int deltaT = p->ticks_used;

  //       // syscall-aware rule
  //       if(deltaS < deltaT && p->current_queue < 3){
  //         p->current_queue++;   // demote CPU-bound process
  //       }
      
  //       p->last_syscall_count = p->syscount;
  //       p->ticks_used = 0;
  //       yield();
  //     }
  //   }

  //   // ===== global priority boost =====
  //   if(global_ticks % 128 == 0){
  //     for(struct proc *q = proc; q < &proc[NPROC]; q++){
  //       acquire(&q->lock);
  //       if(q->state == RUNNABLE){
  //         q->current_queue = 0;
  //       }
  //       release(&q->lock);
  //     }
  //   }
  // }
  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}

//
// set up trapframe and control registers for a return to user space
//
void
prepare_return(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(). because a trap from kernel
  // code to usertrap would be a disaster, turn off interrupts.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // if(which_dev == 2){
  //   struct proc *p = myproc();

  //   if(p && p->state == RUNNING){
      
  //     if(p->ticks_used == 1) {
  //         p->last_syscall_count = getsyscount();
  //       }
  //     //printf("tick pid %d level %d ticks %d\n",
  //          //p->pid, p->current_queue, p->ticks_used);
  //     p->ticks_used++;
  //     global_ticks++;
  //     p->ticks[p->current_queue]++; 

  //     if(p->ticks_used >= mlfq_quantum[p->current_queue]){

  //       int deltaS = getsyscount() - p->last_syscall_count;
  //       int deltaT = p->ticks_used;
  //       //printf("DEMOTE CHECK pid %d deltaS=%d deltaT=%d\n",
  //       //p->pid, deltaS, deltaT);

  //       if(deltaS < deltaT && p->current_queue < 3){
  //         p->current_queue++;
  //       }
  //       p->ticks_used = 0;

  //       //p->last_syscall_count = p->syscount;
        
  //       yield();
  //     }
  //   }

  //   // global boost
  //   if(global_ticks > 0 && global_ticks % 128 == 0){
  //     for(struct proc *q = proc; q < &proc[NPROC]; q++){
  //       acquire(&q->lock);
  //       if(q->state == RUNNABLE){
  //         q->current_queue = 0;
  //       }
  //       release(&q->lock);
  //     }
  //   }
  

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

