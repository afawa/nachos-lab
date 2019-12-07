// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void exec(int pointer){
    char name[20];
    int pos=0;
    int c;
    while(1){
        machine->ReadMem(pointer+pos,1,&c);
        if(c==0){
            name[pos]=0;
            break;
        }
        name[pos++]=(char)c;
    }
    printf("name:%s\n",name);
    OpenFile *openfile = fileSystem->Open(name);
    AddrSpace *space = new AddrSpace(openfile);
    currentThread->space = space;
    delete openfile;
    space->InitRegisters();
    space->RestoreState();
    machine->Run();
}

class Temp{
    public:
    AddrSpace *space;
    int pointer;
    int Next_PC;
};

void fork(int arg){
    Temp* temp=(Temp*) arg;
    AddrSpace* space = temp->space;
    currentThread->space=space;
    int PC = temp->pointer;
    space->InitRegisters();
    space->RestoreState();
    machine->WriteRegister(PCReg,PC);
    machine->WriteRegister(NextPCReg,PC+4);
    machine->Run();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
    //printf("hit_num: %d fail_num: %d rate:%lf\n",machine->TLBhit_num,machine->TLBfail_num,(double)machine->TLBhit_num/(machine->TLBfail_num+machine->TLBhit_num));
   	interrupt->Halt();
    }else if((which == PageFaultException)){
        int address = machine->ReadRegister(BadVAddrReg);
        //printf("Bad addr: 0x%x\n",address);
#ifdef USE_TLB 
        machine->TLBswap(address);
#else
        //倒排页表
        int vpn = (unsigned) address / PageSize;
        int offset = (unsigned) address % PageSize;
        int ppn = machine->SimpleHash(vpn);
        printf("virtualAddr:0x%x vpn:%d ppn:%d\n",address,vpn,ppn);
        OpenFile *openfile = fileSystem->Open(currentThread->getVname());
        if(!machine->pageTable[ppn].valid){
            memMa->allocate(vpn,currentThread,ppn);
            openfile->ReadAt(&(machine->mainMemory[ppn*PageSize]),
                    PageSize,vpn*PageSize);
            machine->pageTable[ppn].virtualPage=vpn;
        }else if(machine->pageTable[ppn].virtualPage!=vpn){
            openfile->WriteAt(&(machine->mainMemory[ppn*PageSize]),
                    PageSize,machine->pageTable[ppn].virtualPage*PageSize);
            memMa->allocate(vpn,currentThread,ppn);
            openfile->ReadAt(&(machine->mainMemory[ppn*PageSize]),
                    PageSize,vpn*PageSize);
        }
        delete openfile;
        machine->pageTable[ppn].virtualPage = vpn;
        machine->pageTable[ppn].valid = true;
        machine->pageTable[ppn].readOnly = false;
        machine->pageTable[ppn].use=false;
        machine->pageTable[ppn].dirty = false;
#endif 
    } else if((which == SyscallException) && (type==SC_Create)){
        printf("Create\n");
        //create
        int name_point = machine->ReadRegister(4);
        char name[20];
        int pos=0;
        int c;
        while(1){
            machine->ReadMem(name_point+pos,1,&c);
            if(c==0){
                name[pos]=0;
                break;
            }
            name[pos++]=(char)c;
        }
        pos=0;
        while(1){
            machine->ReadMem(name_point+pos,1,&c);
            if(c==0){
                name[pos]=0;
                break;
            }
            name[pos++]=(char)c;
        }
        printf("%s\n",name);
        fileSystem->Create(name,128);
        //PC increase
        machine->PC_increase();
    }else if((which == SyscallException) && (type==SC_Open)){
        //open
        printf("Open\n");
        int name_point = machine->ReadRegister(4);
        char name[20];
        int pos=0;
        int c;
        while(1){
            machine->ReadMem(name_point+pos,1,&c);
            if(c==0){
                name[pos]=0;
                break;
            }
            name[pos++]=(char)c;
        }
        OpenFile* openfile = fileSystem->Open(name);
        machine->WriteRegister(2,int(openfile));//返回值
        machine->PC_increase();
    }else if((which==SyscallException) && (type==SC_Close)){
        //close
        printf("Close\n");
        OpenFile* openfile = machine->ReadRegister(4);
        delete openfile;
        machine->PC_increase();
    }else if((which==SyscallException) && (type==SC_Write)){
        //write
        printf("Write\n");
        int pointer = machine->ReadRegister(4);
        int length = machine->ReadRegister(5);
        OpenFile* openfile = machine->ReadRegister(6);
        char buffer[length];
        int c;
        for(int i=0;i<length;++i){
            machine->ReadMem(pointer+i,1,&c);
            buffer[i]=(char)c;
        }
        openfile->Write(buffer,length);
        machine->PC_increase();
    }else if((which==SyscallException)&&(type==SC_Read)){
        //read
        printf("Read\n");
        int pointer = machine->ReadRegister(4);
        int length = machine->ReadRegister(5);
        OpenFile* openfile = machine->ReadRegister(6);
        char buffer[length];
        int real_length = openfile->Read(buffer,length);
        printf("read length: %d\n",real_length);
        for(int i=0;i<real_length;++i){
            machine->WriteMem(pointer+i,1,buffer[i]);
        }
        machine->WriteRegister(2,real_length);
        machine->PC_increase();
    }else if((which==SyscallException)&&(type==SC_Exec)){
        //exec
        printf("Exec\n");
        int pointer = machine->ReadRegister(4);
        char name[20];
        int pos=0;
        int c;
        while(1){
            machine->ReadMem(pointer+pos,1,&c);
            if(c==0){
                name[pos]=0;
                break;
            }
            name[pos++]=(char)c;
        }
        Thread* t=new Thread("Thread1");
        t->Fork(exec,pointer);
        machine->WriteRegister(2,t->getTid());
        machine->PC_increase();
    }else if((which==SyscallException)&&(type==SC_Fork)){
        //fork
        printf("%s fork\n",currentThread->getName());
        int func_pointer = machine->ReadRegister(4);
        OpenFile *openfile=fileSystem->Open(currentThread->filename);
        AddrSpace *space = new AddrSpace(openfile);
        space->CpyAddrSpace(currentThread->space);
        Temp* temp=new Temp;
        temp->space = space;
        temp->pointer=func_pointer;
        temp->Next_PC = machine->registers[PCReg]+4;
        Thread* t=new Thread("Thread1");
        t->Fork(fork,int(temp));
        machine->PC_increase();
    }else if((which==SyscallException)&&(type==SC_Yield)){
        printf("Yield\n");
        machine->PC_increase();
        currentThread->Yield();
    }else if((which==SyscallException)&&(type==SC_Join)){
        //join
        printf("join\n");
        int tid=machine->ReadRegister(4);
        while(tid_alloc[tid])
            currentThread->Yield();
        machine->PC_increase();
    }else if((which==SyscallException)&&(type==SC_Exit)){
        printf("Thread %s Exit\n",currentThread->getName());
        int status = machine->ReadRegister(4);
        machine->PC_increase();
        currentThread->Finish();
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
