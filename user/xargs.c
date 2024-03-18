// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "kernel/param.h"
// #include "user/user.h"

// // 1 为打印调试信息
// #define DEBUG 0

// // 宏定义
// #define debug(codes) if(DEBUG) {codes}

// void xargs_exec(char* program, char** paraments);

// void
// xargs(char** first_arg, int size, char* program_name)
// //first_arg就是xargs中的命令和参数 size命令和参数的个数 
// //xargs函数就是将echo hello too | xargs echo bye中hello too移动到bye的后面
// {
//     char buf[1024];
//     debug(
//         for (int i = 0; i < size; ++i) {
//             printf("first_arg[%d] = %s\n", i, first_arg[i]);
//         }
//     )
//     char *arg[MAXARG];//存放的是指针，arg指向指针的指针

//     //这里m的使用方法值得学习
//     int m = 0;
//     while (read(0, buf+m, 1) == 1) {
//         if (m >= 1024) {
//             fprintf(2, "xargs: arguments too long.\n");
//             exit(1);
//         }
//         if (buf[m] == '\n') {
//             buf[m] = '\0';
//             debug(printf("this line is %s\n", buf);)
//             memmove(arg, first_arg, sizeof(*first_arg)*size);
//             //sizeof(*first_arg)*size 就是整个 first_arg 数组所占的总字节数
//             //first_arg 是一个指向指针的指针，因此 *first_arg 是一个指针

//             // set a arg index
//             int argIndex = size;
//             if (argIndex == 0) {
//                 arg[argIndex] = program_name;//设定默认的echo，并将标准输入的每一行作为参数传递给 "echo"
//                 argIndex++;
//             }
//             //这里的代码逻辑是xargs命令后面有命令 参数x，则下面的代码是再将管道左端0的参数衔接到x后面
//             //如果xargs命令后面没有命令 参数x。定默认的echo，则下面的代码将管道左端0的参数（hello too）衔接进来
//             arg[argIndex] = malloc(sizeof(char)*(m+1));

//             //hello too移动到bye的后面
//             memmove(arg[argIndex], buf, m+1);//m+1限定了读buf的范围是该行，因为buf每行重新开始时，并没有被重置

//             debug(
//                 for (int j = 0; j <= argIndex; ++j)
//                     printf("arg[%d] = *%s*\n", j, arg[j]);
//             )
//             // exec(char*, char** paraments): paraments ending with zero
//             arg[argIndex+1] = 0;//这里的0表示nullptr
//             xargs_exec(program_name, arg);//arg参数是带命令的(命令 参数），program_name是可执行文件路径
//             free(arg[argIndex]);
//             m = 0;
//         } else {
//             m++;
//         }
//     }
// }

// void
// xargs_exec(char* program, char** paraments)
// {
//     if (fork() > 0) {
//         wait(0);
//     } else {
//         debug(
//             printf("child process\n");
//             printf("    program = %s\n", program);
            
//             for (int i = 0; paraments[i] != 0; ++i) {
//                 printf("    paraments[%d] = %s\n", i, paraments[i]);
//             }
//         )
//         if (exec(program, paraments) == -1) {
//             fprintf(2, "xargs: Error exec %s\n", program);
//         }
//         debug(printf("child exit");)
//     }
// }

// int
// main(int argc, char* argv[])
// {
//     debug(printf("main func\n");)
//     char *name = "echo";
//     if (argc >= 2) {
//         name = argv[1];
//         debug(
//             printf("argc >= 2\n");
//             printf("argv[1] = %s\n", argv[1]);
//         )
//     }
//     else {
//         debug(printf("argc == 1\n");)
//     }
//     xargs(argv + 1, argc - 1, name);
//     exit(0);
// }


#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

void xargs_exec(char* program,char** paraments)
{
    if(fork() > 0)
    {
        //printf("wait 前");
        wait(0);
        //printf("wait 后");
    }
    else
    {
        if(exec(program,paraments) == -1)
        {
            fprintf(2, "xargs:error exec\n");
        }
    }
}

void xargs(char** first_arg,int size,char* program_name)
{
    char buf[1024];
    char *arg[MAXARG];//exec的第二个参数
    int m = 0;
    while (read(0, buf + m, 1) == 1)
    {
        if(m > 1023)
        {
            fprintf(2, "command is too long\n");
            exit(1);
        }
        if(buf[m] == '\n')
        {
            buf[m] = '\0';
            memmove(arg, first_arg, sizeof(*first_arg) * size);
            int argIndex = size;
            if(argIndex == 0)
            {
                arg[argIndex] = program_name;
                argIndex++;
            }
            arg[argIndex] = malloc(sizeof(char) * (m + 1));
            memmove(arg[argIndex], buf, m + 1);
            arg[argIndex + 1] = 0;
            xargs_exec(program_name, arg);
            free(arg[argIndex]);
            m = 0;
        }
        else
        {
            m++;
        }
    }
}

int
main(int argc,char *argv[])
{
    char *name = "echo";
    if(argc >= 2)
    {
        name = argv[1];
    }
    xargs(argv + 1, argc - 1, name);
    exit(0);
}
