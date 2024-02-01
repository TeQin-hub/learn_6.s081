#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

//定义一个静态的 volatile 变量 started，用于标识内核是否已经启动。
//volatile 关键字告诉编译器不要对这个变量进行优化，因为它可能会被异步地修改。
volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){//只有编号为 0 的 CPU 执行特定的初始化工作
    consoleinit();//初始化控制台输出
    printfinit();//初始化 printf 库
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator初始化物理页面分配器
    kvminit();       // create kernel page table创建内核页表
    kvminithart();   // turn on paging启用分页
    procinit();      // process table初始化进程表
    trapinit();      // trap vectors初始化中断处理
    trapinithart();  // install kernel trap vector安装内核中断向量
    plicinit();      // set up interrupt controller设置中断控制器
    plicinithart();  // ask PLIC for device interrupts向 PLIC (中断控制器) 请求设备中断.
    binit();         // buffer cache初始化缓冲区缓存
    iinit();         // inode table初始化 inode 表
    fileinit();      // file table初始化文件表
    virtio_disk_init(); // emulated hard disk初始化虚拟硬盘
    userinit();      // first user process启动第一个用户进程

    //__sync_synchronize(); 用于确保 started = 1; 这个赋值操作在其他 CPU 中能够正确地看到。
    //它起到了一个同步的作用，避免了编译器或处理器对这些操作进行乱序执行或优化。
    __sync_synchronize();//是一个同步内存操作的内建函数，用于实现内存屏障或内存同步操作。
    started = 1;//设置 started 为 1，表示内核已启动
  } else {
    while(started == 0)//等待内核的启动
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    
    //这些函数在所有 CPU 上执行
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }
  //所有 CPU 调用 scheduler() 函数，进入调度循环，开始执行进程调度和执行。
  scheduler();        
}
