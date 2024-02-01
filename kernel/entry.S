	# qemu -kernel loads the kernel at 0x80000000
        # and causes each CPU to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
	# set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a0, 1024*4
	csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
	# jump to start() in start.c
        call start
spin:
        j spin
#这段内容可能将cpu的栈内存在物理地址连续的摆放在一起。每个栈内存大小为4096-byte
#硬件线程的编号从0开始，整个系统内，硬件线程具有唯一的编号
#为了方便mul a0, a0, a1，避免第一个的0，将所有硬件编号都+1
#sp指向该cpu的栈内存的起始地址，sp从高地址向低地址移动
#stack0是包含所有cpu的栈空间的数组，该数组的大小就是所有cpu栈空间的字节大小