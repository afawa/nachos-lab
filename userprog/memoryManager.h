#ifndef MEMMAN_H
#define MEMMAN_H

#include "thread.h"

class memoryManager{
    public:
        memoryManager(int pages);
        ~memoryManager();
        bool isAllocate(int page);
        void allocate(int vpn,Thread* t,int page);
        int findPage(int vpn,Thread* t);
        int findFreePage(Thread* t);
        Thread** T;
        int* V;
        int pages;
};

#endif 
