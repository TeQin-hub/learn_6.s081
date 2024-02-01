#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
//__attribute__ ((aligned (16))): 编译器属性（compiler attribute），编译器将变量按照 16 字节对齐。对齐 提高内存访问的效率
//NCPU是 CPU 的数量
//每个 CPU 都会有一个 4096 字节的栈空间，而所有的栈空间会被放在一个数组 stack0 中。
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// a scratch area per CPU for machine-mode timer interrupts.
uint64 timer_scratch[NCPU][5];

// assembly code in kernelvec.S for machine-mode timer interrupt.
extern void timervec();

// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  //设置 Machine Status Register (mstatus) 中的 MPP（Previous Privilege mode）字段，将其设为 Supervisor 模式。
  //为了在之后的 mret 指令中从机器模式切换到 Supervisor 模式。
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  // Machine Exception Program Counter (mepc) 寄存器设置为 main 函数的地址
  // 为了在 mret 指令执行后跳转到 main 函数
  w_mepc((uint64)main);

  // disable paging for now.
  //禁用分页机制 0 即虚拟地址直接映射到物理地址
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  //将 Machine Exception Delegation (medeleg) 和 Machine Interrupt Delegation (mideleg) 寄存器的所有位都设置为 1
  //将所有异常和中断委托给 Supervisor 模式处理。
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  //将 Supervisor Interrupt Enable (sie) 寄存器的相应位设置为 1，使能外部中断 (SEIE)、定时器中断 (STIE) 和软中断（SSIE）。
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  //配置物理内存保护（Physical Memory Protection，PMP），允许 Supervisor 模式访问所有物理内存。
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  //初始化时钟中断，启动系统时钟。
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  //将当前 CPU 的硬件线程 ID 存储在线程指针 (tp) 寄存器中，以便后续在 C 代码中使用。
  //mret 指令会根据之前设置的 mepc 和 mstatus 切换到 Supervisor 模式，
  //同时将程序计数器设置为 mepc 的值。
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  //执行 mret 指令，切换到 Supervisor 模式，并跳转到 main 函数执行。
  //mret 指令会根据之前设置的 mepc 和 mstatus 切换到 Supervisor 模式，同时将程序计数器设置为 mepc 的值。
  asm volatile("mret");
}

// set up to receive timer interrupts in machine mode,
// which arrive at timervec in kernelvec.S,
// which turns them into software interrupts for
// devintr() in trap.c.
void
timerinit()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();

  // ask the CLINT for a timer interrupt.
  int interval = 1000000; // cycles; about 1/10th second in qemu.
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..2] : space for timervec to save registers.
  // scratch[3] : address of CLINT MTIMECMP register.
  // scratch[4] : desired interval (in cycles) between timer interrupts.
  uint64 *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((uint64)scratch);

  // set the machine-mode trap handler.
  w_mtvec((uint64)timervec);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}
