// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

// testnum is set in main.cc
int testnum = 1;
Lock* lock=new Lock("producer consumer lock");
Condition* condition = new Condition("Condition");
int count=0;
const int Maxcount=5;
Semaphore* use=new Semaphore("check use",1);
Semaphore* full=new Semaphore("check full",0);
Semaphore* empty=new Semaphore("check empty",Maxcount);
RWLock* rw_lock=new RWLock("reader writer lock");
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

void SimpleThread1(int which){
    printf("%s here\n",currentThread->getName());
    int num;
    for(num=0;num<10;++num){
        printf("*** thread %d looped %d times\n", which, num);
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
        printf("*** thread %d looped %d times done\n",which,num);
    }
}

void producer(int num){
    for(int i=num;i>0;--i){
        lock->Acquire();
        while(count==Maxcount){
            printf("too many goods. | %s.\n",currentThread->getName());
            condition->Wait(lock);
        }
        printf("Now %d goods. | %s need to produce %d goods.\n",++count,currentThread->getName(),i-1);
        if(count>=1){
            condition->Signal(lock);
        }
        lock->Release();
    }
}

void consumer(int num){
    for(int i=num;i>0;--i){
        lock->Acquire();
        while(count==0){
            printf("No goods. | %s.\n",currentThread->getName());
            condition->Wait(lock);
        }
        printf("Now %d goods. | %s need to consume %d goods.\n",--count,currentThread->getName(),i-1);
        if(count<=Maxcount-1){
            condition->Signal(lock);
        }
        lock->Release();
    }
}

void producer_sema(int num){
    for(int i=num;i>0;--i){
        if(count==Maxcount){
            printf("too many goods. | %s.\n",currentThread->getName());
        }
        empty->P();
        use->P();
        printf("Now %d goods. | %s need to produce %d goods.\n",++count,currentThread->getName(),i-1);
        use->V();
        full->V();
    }
}
void consumer_sema(int num){
    if(interrupt->getLevel()==IntOn)
        printf("On\n");
    else
        printf("Off\n");
    for(int i=num;i>0;--i){
        if(count==0){
            printf("No goods. | %s\n",currentThread->getName());
        }
        full->P();
        use->P();
        printf("Now %d goods. | %s need to produce %d goods.\n",--count,currentThread->getName(),i-1);
        use->V();
        empty->V();
    }
}

void writer(int num){
    rw_lock->Acquire_w();
    count=num;
    printf("%s write %d\n",currentThread->getName(),count);
    currentThread->Yield();
    printf("%s done.\n",currentThread->getName());
    rw_lock->Release_w();
}

void reader(int num){
    rw_lock->Acquire_r();
    printf("%s read %d\n",currentThread->getName(),count);
    currentThread->Yield();
    printf("%s done.\n",currentThread->getName());
    rw_lock->Release_r();
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread* t = Thread::createThread("forked thread");
    //Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}

void ThreadTest2(){
    DEBUG('t', "Entering ThreadTest2");
    Thread* t = Thread::createThread("forked thread 1");
    //Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
    TS();
}

void ThreadTest3(){
    DEBUG('t', "Entering ThreadTest3");
    for(int i=1;i<=10;++i){
        char *prefix="forked thread ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
        Thread* t = Thread::createThread(name);
        t->Fork(SimpleThread,(void*)i);
    }
    SimpleThread(0);
    TS();
}

void ThreadTest4(){
    DEBUG('t', "Entering ThreadTest4");
    for(int i=1;i<=200;++i){
        char *prefix="forked thread ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
        Thread* t = Thread::createThread(name);
        t->Fork(SimpleThread,(void*)i);
    }
    SimpleThread(0);
    TS();
}

void ThreadTest5(){
    DEBUG('t', "Entering ThreadTest5");
    for(int i=1;i<=5;++i){
        char *prefix="forked thread ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
#ifdef PRIORITY 
        Thread *t = Thread::createThread_priority(name,i%3);
#else 
        Thread* t = Thread::createThread(name);
#endif
        t->Fork(SimpleThread,(void*)i);
    }
    SimpleThread(0);
}

void ThreadTest6(){
    DEBUG('t', "Entering ThreadTest6");

    Thread* t = Thread::createThread_priority("forked thread",10);
    //Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread1, (void*)1);
    SimpleThread1(0);
}

void ThreadTest7(){
    DEBUG('t', "Entering ThreadTest7");
    printf("Test 7\n");
    for(int i=1;i<=5;++i){
        char *prefix="forked thread ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
#ifdef PRIORITY 
        Thread *t = Thread::createThread_priority(name,i%3);
#else 
        Thread* t = Thread::createThread(name);
#endif
        t->Fork(SimpleThread1,(void*)i);
    }
    SimpleThread1(0);
}

void pro_con(){
    DEBUG('t',"producer consumer problem.");
    Thread* t1=new Thread("Producer");
    Thread* t2=new Thread("Consumer 1");
    Thread* t3=new Thread("Consumer 2");
    t1->Fork(producer,11);
    t2->Fork(consumer,3);
    t3->Fork(consumer,8);

}
void pro_con_sema(){
    DEBUG('t',"producer consumer problem (sema).");
    Thread* t1=new Thread("Producer");
    Thread* t2=new Thread("Consumer 1");
    Thread* t3=new Thread("Consumer 2");
    t1->Fork(producer_sema,11);
    t2->Fork(consumer_sema,3);
    t3->Fork(consumer_sema,8);
}

void w_r(){
    DEBUG('t',"reader writer problem.");
    for(int i=1;i<=3;++i){
        char *prefix="writer ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
        Thread* t = Thread::createThread(name);
        t->Fork(writer,(void*)i);
    }
    for(int i=1;i<=3;++i){
        char *prefix="reader ";
        char *name = (char*) malloc(strlen(prefix)+3);
        sprintf(name,"%s%d",prefix,i);
        Thread* t = Thread::createThread(name);
        t->Fork(reader,(void*)i);
    }
    int i=4;
    Thread *t = Thread::createThread("writer 4");
    t->Fork(writer,(void*)i);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    ThreadTest5();
    break;
    case 6:
    ThreadTest6();
    break;
    case 7:
    ThreadTest7();
    break;
    case 8:
    pro_con();
    break;
    case 9:
    pro_con_sema();
    break;
    case 10:
    w_r();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

