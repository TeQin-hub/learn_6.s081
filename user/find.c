#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *dir,char *file)
{
    char buf[512], *p;//p表示dir目录下的一个名称（可以是目录或文件）
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(dir,0)) < 0)
    {
        fprintf(2, "find:cannot open %s\n", dir);
        return;//递归调用，不能用exit(1)
    }
    if(fstat(fd,&st) < 0)
    {
        fprintf(2, "find:cannot stat %s\n", dir);
        close(fd);
        return;
    }
    if(st.type != T_DIR)
    {
        fprintf(2, "%s not a director\n", dir);
        close(fd);
        return;
    }
    if(strlen(dir)+1+DIRSIZ+1>sizeof(buf))
    {
        fprintf(2, "find:director is too long\n");
        close(fd);
        return;
    }
    strcpy(buf, dir);//buf绝对路径
    p = buf + strlen(buf);//p指向buf的尾部
    *p++ = '/';
    while(read(fd,&de,sizeof(de)))
    {
        if(de.inum == 0)
        {
            continue;
        }

        /*
        根据 s 指向的字符串小于（s<t）、等于（s==t）或大于（s>t） t 指向的字符串的不同情况
        分别返回负整数、0或正整数
        */
        if(!strcmp(de.name,".")||!strcmp(de.name,".."))
        {
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        //p[DIRSIZ] = '\0';
        // printf("p=%p\n", p);
        // printf("/////\n");
        // printf("p[DIRSIZ]=%p\n", &p[DIRSIZ]);
        *(p + DIRSIZ)='\0';
        // printf("/////\n");
        // printf("p+=%p\n", p);
        //*p = '\0';
        // p++;
        if (stat(buf, &st) < 0)
        {
            fprintf(2, "find:cannot stat %s", buf);
            continue;
        }
        if(st.type == T_DIR)
        {
            find(buf, file);
        }else if(st.type == T_FILE && !strcmp(de.name,file))
        {
            printf("%s\n", buf);
        }
    }
    return;
}

int
main(int argc,char *argv[])
{
    if(argc != 3)
    {
        fprintf(2, "usage: find dirName fileName\n");
        exit(1);//异常退出
    }
    find(argv[1], argv[2]);
    exit(0);
}