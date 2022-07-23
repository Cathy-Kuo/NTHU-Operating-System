// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

void Scheduler::aging(){
	agingCheck(MultiLevelList1);
	agingCheck(MultiLevelList2);
	agingCheck(MultiLevelList3);
}

void Scheduler::agingCheck(List<Thread *> *list){
    ListIterator<Thread*> *iter = new ListIterator<Thread*>((List<Thread*>*)list);
    for( ; iter->IsDone() != true; iter->Next()){
        Thread* now = iter->Item();
        if (now!=kernel->currentThread) {
            now->setAgingCount(now->getAgingCount() + kernel->stats->totalTicks - now->getComeReady());
            now->setComeReady(kernel->stats->totalTicks);
        }
		int oriPriority = now->getPriority();
        if(now->getAgingCount() >= 1500 && oriPriority != 149){
            now->setAgingCount(now->getAgingCount()-1500);
            now->setPriority(now->getPriority() + 10);		
            if(now->getPriority() > 149) now->setPriority(149);
            DEBUG('z', "[C] Tick ["<<kernel->stats->totalTicks << "]: Thread ["<< now->getID() << "] changes its priority from [" << oriPriority << "] to [" << now->getPriority() << "]\n");
            list->Remove(now);
            if(now->getPriority() > 99){
  			    MultiLevelList1->Insert(now);
                if(list != MultiLevelList1){ // L2->L1
                    DEBUG('z',"[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is removed from queue L[2]\n");
			        DEBUG('z',"[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[1]\n");				
				}
            } else if(now->getPriority() > 49){
                MultiLevelList2->Insert(now);
                if(list != MultiLevelList2){ 
 			        DEBUG('z',"[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[2]\n");
                   	DEBUG('z',"[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is removed from queue L[3]\n");
                }
            } else { 
                MultiLevelList3->Append(now);
            }
        }
    }
}

bool Scheduler::Preemtive(){
    if (kernel->currentThread->getPriority() >= 100 && kernel->currentThread->getPriority() <= 149 ){
        if (!MultiLevelList1->IsEmpty()){
            Thread * first_thread = MultiLevelList1->Front();
            double cur_job = kernel->currentThread->getPredict();
            double first_job = first_thread->getPredict();
            if(first_job < cur_job) return true;
        }
    }
    else if (kernel->currentThread->getPriority() >= 50 && kernel->currentThread->getPriority() <= 99){
        if (!MultiLevelList1->IsEmpty()) return true;
    }
    return false;
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

int SJFCompare(Thread *a, Thread *b) {
    if(a->getPredict()/2 + a->getLastTime()/2 == b->getPredict()/2 + b->getLastTime()/2)
        return 0;
    return (a->getPredict()/2 + a->getLastTime()/2) > (b->getPredict()/2 + b->getLastTime()/2) ? 1 : -1;
}
int PriorityCompare(Thread *a, Thread *b) {
    if(a->getPriority() == b->getPriority())
        return 0;
    return (a->getPriority() > b->getPriority()) ? -1 : 1;
}

Scheduler::Scheduler()
{ 
    MultiLevelList1 = new SortedList<Thread *>(SJFCompare);
    MultiLevelList2 = new SortedList<Thread *>(PriorityCompare);
    MultiLevelList3 = new List<Thread *>;
    comingRun = 0;
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete MultiLevelList1;
    delete MultiLevelList2;
    delete MultiLevelList3;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    if(thread->getStatus() == JUST_CREATED){
        thread->setAgingCount(0);
    }
    thread->setStatus(READY);
    thread->setComeReady(kernel->stats->totalTicks);
    
	if(thread->getPriority()>=100 && thread->getPriority()<=149 ){
        DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[1]\n");
        MultiLevelList1->Insert(thread);
    }
    else if(thread->getPriority()>=50 && thread->getPriority()<=99){
        DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[2]\n");
        MultiLevelList2->Insert(thread);
    } 
    else if(thread->getPriority()>=0 && thread->getPriority()<=49){
        DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[3]\n");
        MultiLevelList3->Append(thread);
    } 
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Thread * next_thread;
    if(MultiLevelList1->IsEmpty() == false){
        kernel->alarm->setRoundRobin(false);
        next_thread = MultiLevelList1->RemoveFront();
        DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<next_thread->getID()<<"] is removed from queue L[1]\n");
    } 
	else if(MultiLevelList2->IsEmpty() == false){
        kernel->alarm->setRoundRobin(false);
        next_thread = MultiLevelList2->RemoveFront();
        DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<next_thread->getID()<<"] is removed from queue L[2]\n");
    } 
	else if(MultiLevelList3->IsEmpty() == false){
        kernel->alarm->setRoundRobin(true);
        next_thread = MultiLevelList3->RemoveFront();
        DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<next_thread->getID()<<"] is removed from queue L[3]\n");
    } 
    else return NULL;
    return next_thread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    comingRun = kernel->stats->totalTicks;

    DEBUG('z',"[E] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<nextThread->getID()<<"] is now selected for execution, thread ["
    <<oldThread->getID()<<"] is replaced, and it has executed ["<<oldThread->getAccumExec()<<"] ticks\n");  
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}
