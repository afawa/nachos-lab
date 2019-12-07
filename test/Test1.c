#include "syscall.h"
char buffer[20];
int fd1,fd2,length;

int
main()
{
    Create("test.txt");
    fd1 = Open("read.txt");
    fd2 = Open("test.txt");
    length = Read(buffer,20,fd1);
    Write(buffer,length,fd2);
    Close(fd1);
    Close(fd2);
    Halt();
}
