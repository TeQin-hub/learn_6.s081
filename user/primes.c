#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void mapping(int n,int fd[])
{
    close(n);
    dup(fd[n]);
    close(fd[0]);
    close(fd[1]);
    //exit(0);
}

void primes()
{
    int previousP, nextP;
    if(read(0,&previousP,sizeof(int)))
    {
        printf("prime %d\n", previousP);
        int fd[2];
        pipe(fd);
        if(fork() == 0)
        {
            mapping(1, fd);
            while(read(0,&nextP,sizeof(int)))
            {
                if((nextP % previousP) != 0)
                {
                    write(1, &nextP, sizeof(int));
                }
            }
            //exit(0);
        }
        else
        {
            wait(0);
            mapping(0, fd);
            primes();
        }
    }
}

int
main(int argc,int *argv[])
{
    int fd[2];
    pipe(fd);
    if(fork() == 0)
    {
        mapping(1, fd);
        for (int i = 2; i <= 35;i++)
        {
            write(1, &i, sizeof(int));
        }
        //exit(0);
    }else
    {
        wait(0);
        mapping(0, fd);
        primes();
        //exit(0);
    }
    exit(0);
}