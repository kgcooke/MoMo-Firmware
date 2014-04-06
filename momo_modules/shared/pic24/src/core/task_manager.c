#include "task_manager.h"
#include "common_types.h"
#include "uart.h"
#include "utilities.h"
#include "pic24.h"
#include "memory.h"
#include "bus_master.h"

task_list taskqueue;
void taskloop_init()
{
    ringbuffer_create(&taskqueue.tasks, (void*)taskqueue.taskdata, sizeof(task_item), kMAXTASKS);
    taskqueue.flags = 0;

    taskloop_set_sleep(0);
}

void taskloop_set_sleep(int sleep)
{
    if (sleep)
        SET_BIT(taskqueue.flags, kTaskLoopSleepBit);
    else
        CLEAR_BIT(taskqueue.flags, kTaskLoopSleepBit);
}

int taskloop_add_impl(task_callback task, bool critical )
{
    task_item object;
    object.callback = task;
    object.critical = critical;
    if ( ringbuffer_full( &taskqueue.tasks ) )
        return 0;

    ringbuffer_push( &taskqueue.tasks, &object );

    return 1;
}

int taskloop_add(task_callback task)
{
    return taskloop_add_impl( task, false );
}

int taskloop_add_critical(task_callback task)
{
    return taskloop_add_impl( task, true );   
}

void taskloop_lock()
{
    SET_BIT(taskqueue.flags, kTaskLoopLockedBit);
}
void taskloop_unlock()
{
    CLEAR_BIT(taskqueue.flags, kTaskLoopLockedBit);
}
bool taskloop_locked()
{
    return BIT_TEST(taskqueue.flags, kTaskLoopLockedBit);
}

void taskloop_loop()
{
    while(1)
    {
        while( taskloop_process_one() )
            ;

        if ( BIT_TEST(taskqueue.flags, kTaskLoopSleepBit) )
        {
            //Don't cut-off an rpc master call in progress.
            if (bus_master_idle())
            {
                disable_memory();
                asm_sleep();
            }
        }
    }
}
int taskloop_process_one()
{
    task_item task;

    if (ringbuffer_empty(&taskqueue.tasks))
        return 0;

    ringbuffer_pop(&taskqueue.tasks, &task);
    if ( taskloop_locked() && !task.critical )
        ringbuffer_push( &taskqueue.tasks, &task );
    else
        task.callback();

    return 1;
}
