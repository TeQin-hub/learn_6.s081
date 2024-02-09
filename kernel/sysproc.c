#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 startaddr;//用户的虚拟页面空间
  int npage;//虚拟页面空间 页数
  uint64 useraddr;//接收bitmask(位掩码)的 在用户空间的虚拟地址
  argaddr(0,&startaddr);
  argint(1,&npage);
  argaddr(2,&useraddr);

  uint64 bitmask = 0;
  uint64 complement = ~PTE_A;//complement表示pgaccess操作结束了，需要将被操作的页面的访问位PTE_A重新置为0
  struct proc *p = myproc();
  for(int i=0;i<npage;++i){
    pte_t *pte = walk(p->pagetable,startaddr + i*PGSIZE,0);
    if(*pte & PTE_A){
      bitmask |= (1<<i);
      *pte &= complement;
    }
  }
  copyout(p->pagetable,useraddr,(char *)&bitmask,sizeof(bitmask));
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
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
