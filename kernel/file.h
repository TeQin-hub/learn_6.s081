struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  uint dev;           // Device number设备号，表示 inode 所在的设备
  uint inum;          // Inode number inode 编号，表示 inode 在文件系统中的唯一标识
  int ref;            // Reference count引用计数，表示有多少个指向该 inode 的引用
  struct sleeplock lock; // protects everything below here睡眠锁，用于保护该 inode 下面的所有字段，确保多个线程对 inode 的并发访问安全
  int valid;          // inode has been read from disk?表示 inode 是否已经从磁盘读取过

  short type;         // copy of disk inode文件类型，如普通文件、目录等
  short major;        //设备文件的主设备号
  short minor;        //设备文件的次设备号
  short nlink;        //文件的链接数
  uint size;          //文件大小
  uint addrs[NDIRECT+1]; //存储文件的数据块地址，包括直接块和间接块。其中 NDIRECT 为直接块的数量，加上一个间接块
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
