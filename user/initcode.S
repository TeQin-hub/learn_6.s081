# Initial process that execs /init.该汇编代码表示将执行 /init 程序的初始进程。
# This code runs in user space.指示接下来的代码意味着在用户空间中运行。

#include "syscall.h"
#系统调用号

# exec(init, argv)
.globl start
start:
        la a0, init # 将字符串 init 的地址加载到寄存器 a0 中
        la a1, argv # 将数组 argv 的地址加载到寄存器 a1 中
        li a7, SYS_exec #将系统调用号为 exec 的立即数加载到寄存器 a7 中
        ecall #使用 ecall 指令触发系统调用，这将转移控制到内核

# for(;;) exit(); 指示以下代码表示一个无限循环，反复调用 exit 系统调用
exit:
        li a7, SYS_exit #将系统调用号为 exit 的立即数加载到寄存器 a7 中
        ecall #触发系统调用，导致进程退出
        jal exit #跳转并链接到标签 exit，创建一个无限循环

# char init[] = "/init\0";指示定义一个以空字符结尾的字符串
init:
  .string "/init\0"

# char *argv[] = { init, 0 };指示定义一个包含 init 地址和空指针的数组 argv
.p2align 2 #将接下来的数据对齐到 2 字节边界
argv:
  .long init #在数组中存储字符串 init 的地址
  .long 0 #在数组中存储空指针，用于终止数组
