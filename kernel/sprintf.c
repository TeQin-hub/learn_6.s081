#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "riscv.h"
#include "defs.h"

static char digits[] = "0123456789abcdef";

static int
sputc(char *s, char c)
{
  *s = c;
  return 1;
}

static int
sprintint(char *s, int xx, int base, int sign)
{
  char buf[16];
  int i, n;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  n = 0;
  while(--i >= 0)
    n += sputc(s+n, buf[i]);
  return n;
}

int
snprintf(char *buf, int sz, char *fmt, ...)
{

  //typedef __gnuc_va_list va_list;
  /*
  __gnuc_va_list 是一个由 GCC（GNU Compiler Collection）定义的类型，用于表示可变参数列表。
  这个类型在不同的编译器中可能会有所不同。
  通过 typedef 关键字将其重新命名为 va_list，使得在代码中使用起来更加方便和可移植。
  */
  va_list ap;
  
  int i, c;
  int off = 0;
  char *s;

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);

  //0xff是一个十六进制字面量，它表示一个八位全为1的二进制数
  //&运算符将格式字符串中的每个字符与0xff进行按位与操作。
  //这样做的目的是确保字符被转换为一个范围在0到255之间的整数值。
  for(i = 0; off < sz && (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){

      // 写入字符到缓冲区buf
      off += sputc(buf+off, c);
      continue;
    }

    // 如果遇到 '%' 字符
    // 获取下一个字符
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      off += sprintint(buf+off, va_arg(ap, int), 10, 1);
      break;
    case 'x':
      off += sprintint(buf+off, va_arg(ap, int), 16, 1);
      break;
    case 's':
   // #define va_arg(v,l)	__builtin_va_arg(v,l)
   //定义了va_arg这个宏，它的作用是使用内置函数__builtin_va_arg来获取可变参数列表中的下一个参数。
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s && off < sz; s++)
        off += sputc(buf+off, *s);
      break;
    case '%':
      off += sputc(buf+off, '%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      off += sputc(buf+off, '%');
      off += sputc(buf+off, c);
      break;
    }
  }
  return off;
}
