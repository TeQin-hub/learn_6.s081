#!/usr/bin/perl -w
#这是脚本的开头，指定了使用 Perl 解释器来执行这个脚本，并开启了 Perl 的警告模式 -w，用于产生警告信息。

# Generate usys.S, the stubs for syscalls.

print "# generated by usys.pl - do not edit\n";

print "#include \"kernel/syscall.h\"\n";

sub entry {
    my $name = shift;
    print ".global $name\n";
    print "${name}:\n";
    print " li a7, SYS_${name}\n";
    print " ecall\n";# 调用 ecall 指令触发系统调用
    print " ret\n";
}

#执行 ecall 指令会跳转到 kernel/syscall.c 中 syscall 那个函数处，执行此函数。
#而syscall会将相应的系统调用号放入a7
#所以pl文件生成的usys.S文件， usys.S 文件就是系统调用用户态和内核态的切换接口。
	
entry("fork");
entry("exit");
entry("wait");
entry("pipe");
entry("read");
entry("write");
entry("close");
entry("kill");
entry("exec");
entry("open");
entry("mknod");
entry("unlink");
entry("fstat");
entry("link");
entry("mkdir");
entry("chdir");
entry("dup");
entry("getpid");
entry("sbrk");
entry("sleep");
entry("uptime");

#add
entry("trace");
entry("sysinfo");
