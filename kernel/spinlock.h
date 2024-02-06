// Mutual exclusion lock.
struct spinlock {
  //用于指示锁当前是否被某个线程持有。当 locked 的值为 0 时，表示锁没有被持有，可以被获取；
  //当 locked 的值为 1 时，表示锁已经被某个线程持有，其他线程需要等待。
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  
  //表示当前持有锁的 CPU。在多处理器系统中，这个成员可以用于跟踪哪个 CPU 持有了锁。
  struct cpu *cpu;   // The cpu holding the lock.
};

