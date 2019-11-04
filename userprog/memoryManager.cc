#include "memoryManager.h"
memoryManager::memoryManager(int pages){
    T=new Thread*[pages];
    V=new int[pages];
    this->pages=pages;
    for(int i=0;i<pages;++i){
        T[i]=NULL;
        V[i]=-1;
    }
}
memoryManager::~memoryManager(){
    delete [] T;
    delete [] V;
}
bool memoryManager::isAllocate(int page){
    if(T[page]!=NULL)
        return true;
    return false;
}
void memoryManager::allocate(int vpn,Thread* t,int page){
    T[page]=t;
    V[page]=vpn;
}
int memoryManager::findPage(int vpn,Thread* t){
    for(int i=0;i<this->pages;++i){
        if(T[i]==t&&V[i]==vpn){
            return i;
        }
    }
    return -1;
}
int memoryManager::findFreePage(Thread* t){
    int i;
    for(i=0;i<this->pages;++i){
        if(!isAllocate(i))
            return i;
    }
    
    return -1;
}
