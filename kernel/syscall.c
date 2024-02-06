#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

/*
这两个函数的目的是通过页表（pagetable）从进程的内存中获取数据，确保操作的合法性，以及处理复制失败的情况。
在操作系统中，由于不同进程的虚拟地址空间是隔离的，因此需要通过页表来进行虚拟地址到物理地址的映射。
这样的操作是系统调用和用户空间代码之间进行数据传递的基础。
*/
// Fetch the uint64 at addr from the current process.
//addr 表示要获取数据的地址
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
    
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
  //该函数负责从进程的页表 p->pagetable 中复制数据到指定的地址 ip;内核读入数据
    return -1;
  return 0;
}

// Fetch the nul-terminated(空字符结尾) string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  return strlen(buf);
}

static uint64
argraw(int n)//
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);//指针 ip 用于存储获取到的参数值。
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
//接受三个参数，整数 n 表示第几个系统调用参数，字符数组 buf 用于存储获取到的字符串，整数 max 表示最大复制的长度。
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)//获取字符串的地址
    return -1;
  return fetchstr(addr, buf, max);//从用户空间将字符串复制到内核空间，返回复制的字符串长度。失败返回-1
}

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
extern uint64 sys_trace(void);//add
extern uint64 sys_sysinfo(void);//add
//这是一个包含函数指针的数组，用于映射系统调用号（syscall number）到相应的系统调用处理函数。
//数组的索引就是这个系统调用号,数组中的每个元素都是一个指向返回类型为 uint64 的函数的指针
//当用户程序发起一个系统调用时，内核会根据系统调用号查找对应的处理函数，然后执行这个处理函数来完成相应的操作
//syscalls[SYS_fork] 就是数组 syscalls 中存储了处理 SYS_fork 类型系统调用的函数指针的地方
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_sysinfo] sys_sysinfo,
};

static char *sysname[] = {
  "",//系统调用号从1开始，所以需要一个空字符串占位
  "fork",
  "exit",
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "trace",
  "sysinfo",
};

void
syscall(void)
{
  int num;
  struct proc *p = myproc();//获取当前进程结构体指针

  num = p->trapframe->a7;//得到系统调用号
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();//通过调用 syscalls[num](); 函数，把返回值保存在了 a0 寄存器中

    //add
    if(p->tracemask & (1 << num)){
      printf("%d: syscall %s -> %d\n",p->pid,sysname[num],p->trapframe->a0);
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
