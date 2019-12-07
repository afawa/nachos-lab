#include "syscall.h"
int id;
void func(){
    Create("test1.txt");
    Exit(0);
}
int
main()
{
    id=Exec("./test/halt");
    //Yield();
    //Join(id);
    Create("test2.txt");
    Fork(func);
    Yield();
    Exit(0);
}
