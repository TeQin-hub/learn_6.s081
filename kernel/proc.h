// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state进程的结构体
struct proc {
  struct spinlock lock;
  /*用于保护这个进程结构体的互斥锁。在多线程或多进程环境中，
    当一个进程访问或修改这个结构体时，需要先获得这个锁，
    以防止其他进程同时进行访问导致数据不一致。

    父子进程通信：当父进程希望等待子进程的完成时，它需要访问子进程的信息，例如子进程的状态和退出状态。
    进程调度：调度器可能需要查看系统中所有进程的状态
  */

  // p->lock must be held when using these: p 是 struct proc 的一个实例，即一个进程的实例
  enum procstate state;        // Process state：运行（RUNNING） 就绪（RUNNABLE） 睡眠（SLEEPING）
  void *chan;                  // If non-zero, sleeping on chan非零，表示进程正在等待某个通道（channel），即睡眠状态
  int killed;                  // If non-zero, have been killed非零，表示进程已经被杀死
  int xstate;                  // Exit status to be returned to parent's wait用于存储进程退出时的状态，将会返回给父进程的 wait 系统调用
  int pid;                     // Process ID进程的唯一标识符，即进程 ID。

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process指向父进程的指针。

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack进程的内核栈的虚拟地址
  uint64 sz;                   // Size of process memory (bytes)进程内存的大小，以字节为单位
  pagetable_t pagetable;       // User page table用户页表，用于将用户虚拟地址映射到物理地址
  struct trapframe *trapframe; // data page for trampoline.S指向存储中断帧（trap frame）的结构体的指针，用于保存进程的状态（PC、寄存器、栈指针等），用于恢复进程的执行状态
  struct context context;      // swtch() here to run process用于保存进程上下文的结构体，包括 CPU 寄存器的状态等
  struct file *ofile[NOFILE];  // Open files进程打开的文件数组，每个元素是一个指向文件结构体的指针
  struct inode *cwd;           // Current directory当前工作目录的指针
  char name[16];               // Process name (debugging)进程的名字，用于调试目的
};
