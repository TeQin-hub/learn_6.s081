#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
 这个程序的作用是启动一个跟踪程序，然后执行用户指定的命令。
 如果某些步骤失败，程序会打印相应的错误信息并退出。
*/
int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  //创建一个新的参数数组 nargv，用于存放要传递给 exec 函数的参数。
  //从 argv[2] 开始复制命令行参数，遍历到数组的末尾或达到最大参数数量
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
