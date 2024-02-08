// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

//空闲页链表元素
struct run {
  struct run *next;
};

/*
kmem 可以被理解为内核内存管理的结构，包含了一个自旋锁和一个空闲页面链表。
这种结构通常在内核中用于协调对共享资源的访问，以及管理可用的物理内存页面。
*/
struct {
  //自旋锁是一种轻量级的锁，它不会引起线程阻塞，而是在获取锁失败时一直自旋等待。这是一种常用于内核的锁机制。
  struct spinlock lock;
  //这个链表被用作内存分配的空闲列表，存储着可用的物理内存页面。
  struct run *freelist;
} kmem;

//通过调用freerange来添加内存到空闲链表
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);//向上舍入到最近的页面边界
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)//与kfree配合，将空闲页插入到空闲链表（头插法）
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
//用于释放一个指向物理内存页面的指针所指向的内存页，并将其添加到内存空闲列表中
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  //将要释放的物理内存页填充为特定的值，以便检测悬空引用。这个操作通常被称为"poisoning"，用于在释放内存后避免悬空引用
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;//将物理地址pa转换为结构体run的指针

  acquire(&kmem.lock);//获取内核内存锁
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
//物理内存分配器
//实现了从内核的内存池中分配一页物理内存的操作
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);//确保在对内存池进行操作时是原子的，防止多个线程同时访问导致的竞态条件。
  r = kmem.freelist;//将 kmem.freelist 指向的第一个空闲页面取出，赋值给 r。
  if(r)
    kmem.freelist = r->next;//取下第一个空闲页面
  release(&kmem.lock);//释放内存管理结构的锁，允许其他线程再次访问内存池。

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

//add lab2sysinfo 空闲的内核内存
uint64
free_mem(void)
{
  struct run *r;
  uint64 num = 0;
  acquire(&kmem.lock);
  r=kmem.freelist;
  while(r)
  {
    num++;
    r=r->next;
  }
  release(&kmem.lock);
  return num*PGSIZE;
}