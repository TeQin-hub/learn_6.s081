// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGICELF 文件的魔数，用于验证文件是否符合 ELF 文件格式。必须等于预定义的常量 ELF_MAGIC
  uchar elf[12];//存储 ELF 文件头的其他标识信息
  ushort type;//ELF 文件的类型，指示文件是可执行文件、目标文件还是共享对象
  ushort machine;//目标处理器的体系结构类型
  uint version;//ELF 文件的版本号
  uint64 entry;//程序的入口点，即程序开始执行的地址
  uint64 phoff;//程序头表在文件中的偏移量（以字节为单位）
  uint64 shoff;//节头表在文件中的偏移量（以字节为单位）
  uint flags;//与文件相关的标志位
  ushort ehsize;//ELF 文件头的大小（以字节为单位）
  ushort phentsize;//程序头表中每个条目的大小（以字节为单位）
  ushort phnum;//程序头表中条目的数量
  ushort shentsize;//节头表中每个条目的大小（以字节为单位）
  ushort shnum;//节头表中条目的数量
  ushort shstrndx;//字符串表节索引，指示节头表中用于存储节名称的节的索引
};

// Program section header
struct proghdr {
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
