#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200 //te:打开文件时，文件不存在，则创建文件
#define O_TRUNC   0x400 //te:打开文件时，如果文件已经存在，将其长度截断为0，用于清空已有文件内容
