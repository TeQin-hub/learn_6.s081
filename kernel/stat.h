#define T_DIR     1   // Directory
#define T_FILE    2   // File
#define T_DEVICE  3   // Device

struct stat {
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short type;  // Type of file，可以是目录、文件或设备
  short nlink; // Number of links to file
  uint64 size; // Size of file in bytes
};

//定义文件系统中的文件类型和文件状态结构体