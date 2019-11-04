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
    } 
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
