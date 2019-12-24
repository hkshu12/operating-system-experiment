
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
  PROCESS *p;
  int greatest_ticks = 0;

  while (!greatest_ticks)
  {
    for (p = proc_table; p < proc_table + NR_TASKS; p++)
    {
      if (p->ticks > greatest_ticks && !p->sleep_time && !p->wait)
      {
        greatest_ticks = p->ticks;
        p_proc_ready = p;
      }
    }

    if (!greatest_ticks)
    {
      for (p = proc_table; p < proc_table + NR_TASKS; p++)
      {
        p->ticks = p->priority;
      }
    }
  }
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
  return ticks;
}

/*======================================================================*
                           sys_print
 *======================================================================*/
PUBLIC void sys_print(char *info)
{
  disp_str(info);
}
/*======================================================================*
                           sys_color_print
 *======================================================================*/
PUBLIC void sys_color_print(char *info, int color)
{
  disp_color_str(info, color);
}
/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds)
{
  p_proc_ready->sleep_time = milli_seconds / 10;
  schedule();
}
/*======================================================================*
                           sys_p
 *======================================================================*/
PUBLIC void sys_p(SEMAPHORE *semaphore)
{
  semaphore->count--;
  if (semaphore->count < 0)
  {
    p_proc_ready->wait = 1;
    semaphore->queue[semaphore->p_head] = p_proc_ready->pid;
    semaphore->p_head = (semaphore->p_head + 1) % NR_TASKS;
    schedule();
  }
}
/*======================================================================*
                           sys_v
 *======================================================================*/
PUBLIC void sys_v(SEMAPHORE *semaphore)
{
  semaphore->count++;
  if (semaphore->count <= 0)
  {
    (proc_table[semaphore->queue[semaphore->p_tail]]).wait = 0;
    semaphore->queue[semaphore->p_tail] = -1;
    semaphore->p_tail = (semaphore->p_tail + 1) % NR_TASKS;
  }
}
