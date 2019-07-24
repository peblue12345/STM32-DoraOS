#include <mutex.h>
#include <log.h>
#include <mem.h>
#include <string.h>
#include <dos_config.h>
#include <task.h>
#include <port.h>


/** 
 * create a mutex 
 */
Dos_Mutex_t Dos_MutexCreate(void)
{
    Dos_Mutex_t mutex;

    mutex = Dos_MemAlloc(sizeof(struct Dos_Mutex));
    if(mutex == DOS_NULL)
    {
        DOS_LOG_ERR("mutex is null\n");
        return DOS_NULL;
    }

    memset(mutex, 0, sizeof(struct Dos_Mutex));

    mutex->MutexCnt = 0;
    mutex->MutexOwner = DOS_NULL;
    mutex->Priority = 0;
    
    /** Initialize mutex pend list */
    Dos_TaskList_Init(&(mutex->MutexPend));

    return mutex;
}

/**
 * delete a mutex
 */
dos_err Dos_MutexDelete(Dos_Mutex_t mutex)
{
    if(mutex != DOS_NULL)
    {
        if(Dos_TaskList_IsEmpty(&(mutex->MutexPend)))
        {
            memset(mutex,0,sizeof(struct Dos_Mutex));
            Dos_MemFree(mutex);
            return DOS_OK;
        }
        else
        {
            DOS_LOG_WARN("there are tasks in the mutex pend list\n");
            return DOS_NOK;
        }
    }
    else
    {
        DOS_LOG_WARN("the mutex to be deleted is null\n");
        return DOS_NOK;
    }
}

/**
 * the mutex P operation, pending mutex
 */
dos_err Dos_MutexPend(Dos_Mutex_t mutex, dos_uint32 timeout)
{
    DOS_TaskCB_t task;
    dos_uint32 pri;

    if((mutex == DOS_NULL) || (Dos_ContextIsInt()))
    {
        DOS_LOG_WARN("unable to continue to pend the mutex, the mutex is null or is currently in the interrupt context\n");
        return DOS_NOK;
    }

    task = (DOS_TaskCB_t)Dos_Get_CurrentTCB();
    
    pri = Dos_Interrupt_Disable();
    /** can pend the mutex */
    if(mutex->MutexCnt == 0)
    {
        mutex->MutexCnt++;
        mutex->Priority = task->Priority;
        mutex->MutexOwner = task;
        Dos_Interrupt_Enable(pri);
        return DOS_OK;
    }

    if(mutex->MutexOwner == task)
    {
        mutex->MutexCnt++;
        Dos_Interrupt_Enable(pri);
        return DOS_OK;
    }

    /** time is be set 0 or scheduler is lock */
    if((timeout == 0) || (Dos_Scheduler_IsLock()))  
    {
        Dos_Interrupt_Enable(pri);
        return DOS_NOK;
    }

    /** current task has a higher priority, priority inheritance is required */
    if(mutex->Priority > task->Priority) 
    {
        Dos_SetTaskPrio(mutex->MutexOwner, task->Priority);
    }

    Dos_TaskWait(&mutex->MutexPend, timeout);
    Dos_Interrupt_Enable(pri);
    Dos_Scheduler();
    
    /** Task resumes running */
    if(task->TaskStatus & DOS_TASK_STATUS_TIMEOUT)
    {
        pri = Dos_Interrupt_Disable();
        DOS_RESET_TASK_STATUS(task, (DOS_TASK_STATUS_TIMEOUT | DOS_TASK_STATUS_SUSPEND));
        DOS_SET_TASK_STATUS(task, DOS_TASK_STATUS_READY);
        Dos_TaskItem_Del(&(task->PendItem));
        DOS_LOG_INFO("waiting for mutex timeout\n");
        Dos_Interrupt_Enable(pri);
        return DOS_NOK;
    }

    return DOS_OK;
}


dos_err Dos_MutexPost(Dos_Mutex_t mutex)
{
    DOS_TaskCB_t task;
    dos_uint32 pri;

    if((mutex == DOS_NULL) || (Dos_ContextIsInt()))
    {
        DOS_LOG_WARN("unable to continue to post the mutex, the mutex is null or is currently in the interrupt context\n");
        return DOS_NOK;
    }

    task = (DOS_TaskCB_t)Dos_Get_CurrentTCB();
    
    pri = Dos_Interrupt_Disable();

    if((mutex->MutexCnt == 0) || (mutex->MutexOwner != task))
    {
        DOS_LOG_WARN("unable to continue to post the mutex, the mutex is not be held, or the owner is not the current task\n");
        Dos_Interrupt_Enable(pri);
        return DOS_NOK;
    }


    if(--(mutex->MutexCnt) != 0)
    {
        Dos_Interrupt_Enable(pri);
        return DOS_OK;
    }

    /** Determine if priority inheritance occurs ? */
    if(mutex->MutexOwner->Priority != mutex->Priority)
    {
        Dos_SetTaskPrio(mutex->MutexOwner, mutex->Priority);
    }

    if(!Dos_TaskList_IsEmpty(&(mutex->MutexPend)))
    {
        task = Dos_GetTCB(&(mutex->MutexPend));
        mutex->Priority = task->Priority;
        mutex->MutexCnt++;
        mutex->MutexOwner = task;
        Dos_TaskWake(task);
        Dos_Interrupt_Enable(pri);
        Dos_Scheduler();
    }
    else
    {
        mutex->Priority = 0;
        mutex->MutexOwner = DOS_NULL;
    }

     Dos_Interrupt_Enable(pri);

     return DOS_OK;
}





