#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t
kvmmake(void)
{
  pagetable_t kpgtbl;

  kpgtbl = (pagetable_t) kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  // map kernel stacks
  proc_mapstacks(kpgtbl);
  
  return kpgtbl;
}

// Initialize the one kernel_pagetable
void
kvminit(void)
{
  kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
// 如果在查找的过程中某一级的页表项不存在（即 *pte & PTE_V 为假），并且需要分配新的页表页（alloc 为真），
//则动态分配一个新的页表页。这样可以在需要时延迟分配内存，而不是一开始就为整个地址空间分配所有的页表页。
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");//宕机（panic）条件，表示程序进入了一个无法恢复的错误状态。

//通过一个循环迭代，从页表的高级别向低级别逐步查找页表项。RISC-V Sv39 模式下有三个级别的页表。
  for(int level = 2; level > 0; level--) {//Level-2 是最宽范围的
    /*
     PX(level, va) 是一个宏，用于计算给定级别 level 和虚拟地址 va 的页表项在当前级别页表中的索引
     &pagetable[PX(level, va)] 表示取得页表中索引为 PX(level, va) 的页表项的地址
     pte 是一个指向虚拟地址 va 在给定级别 level 的页表项的指针。可以通过 pte 来读取或修改该页表项的内容。
    */
    pte_t *pte = &pagetable[PX(level, va)];

    if(*pte & PTE_V) {//页表项存在且有效
      //pagetable 被更新为下一级页表的物理地址，以便继续迭代查找。
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {//如果页不存在看是否alloc创建新的页
      /*
       alloc 为0，表示不需要分配新的页表页，用于指示是否需要分配内存。
       || 是逻辑或运算符，表示或者。如果前面的条件 !alloc 为真，那么整个条件语句就为真，不再执行后续的操作
       alloc 为真，那么将执行 pagetable = (pde_t*)kalloc()。这行代码的目的是分配内核空间，用于存储新的页表页。
       kalloc() 是一个函数，用于从内核空间分配一块页面大小的内存。
       == 0 是比较操作符，用于检查是否成功分配内存。如果分配失败，kalloc() 将返回0，此时整个条件语句为真，表示分配失败。

       如果不需要分配新的页表页（!alloc 为真），或者尝试分配新的页表页但分配失败（kalloc() 返回0），则返回0，
       表示无法创建新的页表页。这样的设计可以用于避免在某些情况下分配内存失败时继续执行可能导致错误的操作。
      */
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;

      /*
       memset 用于将 pagetable 指向的内存块的内容全部设置为0。
       pagetable：指向内存块的指针，即要被设置的起始地址
       PGSIZE：要设置的字节数，即内存块的大小。
      */
      memset(pagetable, 0, PGSIZE);

      /*
       PA2PTE(pagetable)：这是一个宏或函数，用于将一个物理地址 pagetable 转换为页表项格式。
       具体的实现可能会将物理地址左移12位，并设置其他位的标志，以创建一个符合页表项格式的值。

       PTE_V：这是页表项中的一个标志，表示有效位（Valid）。将该标志设置为1表示该页表项对应的映射是有效的。

       |：位运算中的按位或操作符，将上述两个值按位进行或运算，得到一个组合了有效位和转换后的物理地址的新页表项值。

       *pte 这个页表项设置为指向 新分配的页表 页的物理地址，并将有效位标志设置为1，表示建立了有效的映射。
      */
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];//返回指向虚拟地址对应的页表项的指针，是最底层的页表中的页表项
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  //指向页表项的指针 pte 和用于存储物理地址的变量 pa
  pte_t *pte;
  uint64 pa;

//检查虚拟地址 va MAXVA 是一个常量，代表了虚拟地址的上限
  if(va >= MAXVA)
    return 0;

//walk 函数查找虚拟地址 va 对应的页表项。walk 函数通常用于在页表中进行查找，并返回指向页表项的指针
  pte = walk(pagetable, va, 0);

//检查页表项的有效性。如果找不到页表项 (pte == 0) 
//或者找到的页表项中 PTE_V 标志（有效位）被清零，表示虚拟地址未映射，函数返回 0。
  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;

//如果页表项中 PTE_U 标志（用户权限位）被清零，
//表示这是一个内核空间的地址，而不是用户空间的地址，函数返回 0
  if((*pte & PTE_U) == 0)
    return 0;

//如果通过以上检查，说明虚拟地址有效且已映射到物理地址。
//通过宏 PTE2PA 从页表项中提取出物理地址，并返回该物理地址。
  pa = PTE2PA(*pte);
  return pa;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kpgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  if(size == 0)
    panic("mappages: size");
  
  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V)
      panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0)
      panic("uvmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
  memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for(a = oldsz; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
//pagetable是页表，dst是目标内核缓冲区的指针，srcva是用户空间的虚拟地址(在pagetable页表中)，max是最大允许复制的字节数
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  //n（要复制的字节数）、va0（虚拟地址对应的页面的起始地址）、pa0（虚拟地址对应的页面的物理地址）
  //got_null（用于标记是否已经复制到null字符）
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    //获取当前虚拟地址所在页面的起始地址和对应的物理地址
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);

    if(pa0 == 0)//如果物理地址为0，表示页面不存在或不可访问，返回错误码-1
      return -1;

    //计算页面还剩多少内容可以复制，如果n大于max，则是复制max的内容，如果n小于max，则在本页先复制n个内容
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));//将物理地址对应的缓冲区转换为字符指针
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;//更新srcva，指向下一个页面的起始地址
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}
