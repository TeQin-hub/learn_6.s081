#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)//格式化文件或目录名称以进行显示的函数
{
  static char buf[DIRSIZ+1];// 静态数组（全局）用于存储格式化后的文件名
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;//目录是一个包含一系列dirent结构的文件
  struct stat st;//表示管理文件的属性

//打开由 'path' 指定的目录
  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){//系统调用fstat,获取文件描述符fd指向的文件情况
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE://fd是一个文件
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR://fd是一个目录，则要处理fd目录下的所有条目，这些条目可以是目录或者文件，由68行代码进行输出
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){//path只包含文件或目录的路径
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    //逐个读取目录条目
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)//有时目录项可能被删除，而文件系统并未立即清除相关的 inode
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){//打开文件路径的文件的信息
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
      //目录和文件都是只输出： 名称（不含前面的路径，即最后一个/的后一个字符串） 类型 inode 文件大小（目录为0）
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
