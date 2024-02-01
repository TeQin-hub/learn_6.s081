// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);//如果打开失败，说明该文件不存在，因此通过 mknod 创建一个名为 "console" 的设备节点。
    open("console", O_RDWR);
  }

  /*
    将文件描述符 0（标准输入）复制到文件描述符 1（标准输出）和文件描述符 2（标准错误）。
    这样，进程的标准输出和标准错误都被重定向到 "console"。
  */
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){//一个无限循环，表示 init 进程将一直尝试运行 shell
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);//用新程序 "sh" 替换当前进程的镜像，传递参数列表 argv
      printf("init: exec sh failed\n");
      exit(1);//如果 exec 调用失败，打印错误信息并退出子进程
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      //等待任意子进程退出，wpid 存储退出的子进程的 PID
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;//跳出内层的for循环，继续外层的。以保持系统中至少有一个 shell 进程在运行
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.这是一个无父进程的进程；不执行任何操作
        /*
          init 进程通过 fork 创建子进程，并使用 wait 来等待子进程的退出。
          当 wait 返回的 PID 不等于之前创建的子进程的 PID 时，表示这是一个无父进程的进程。
          由于 init 的主要任务是启动并监控 shell 进程，对于无父进程的其他进程，init 不进行额外的操作。
        */
      }
    }
  }
}
