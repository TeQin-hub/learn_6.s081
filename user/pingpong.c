#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc,char *argv[])
{
    int pid;
    char buf[] = {'a'};//给定一个IO缓冲区
    int pipes1[2], pipes2[2];
    int ret = fork();

    pipe(pipes1);
    pipe(pipes2);

    /*
    pipes[0]读数据，pipes[1]写数据
    pipes1 :父进程传数据到子进程
    pipes2 :子进程传数据到父进程
    */
    if(ret == 0)
    {
        //子进程
        pid = getpid();
        close(pipes1[1]);
        //close(pipes2[0]);
        read(pipes1[0], buf, 1);
        printf("%d: received ping\n", pid);
        write(pipes2[1], buf, 1);
        exit(0);//子进程一定要使用exit(0)
    }
    else
    {
        //父进程
        pid = getpid();
        //close(pipes1[0]);
        close(pipes2[1]);
        write(pipes1[1], buf, 1);
        wait(0);//非常重要!!!否则会因为进程切换，导致输出乱序；父wait与子exit要成对出现
        read(pipes2[0], buf, 1);
        printf("%d: received pong\n", pid);
        exit(0);//父进程也需要调用
    }
}
