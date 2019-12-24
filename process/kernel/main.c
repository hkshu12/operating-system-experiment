
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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

// #define READER

#define NUM_OF_READERS 3

/* 新建信号量 */
/* 读者优先*/
static SEMAPHORE rmutex;
static SEMAPHORE wmutex;
static SEMAPHORE mutex;
/* 写者优先*/
static SEMAPHORE semaphore;
static int readCount;

void initSemaphore(SEMAPHORE *semaphore, int count)
{
  semaphore->count = count;
  for (int i = 0; i < 6; i++)
  {
    semaphore->queue[i] = -1;
  }
  semaphore->p_head = 0;
  semaphore->p_tail = 0;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
  disp_str("-----\"kernel_main\" begins-----\n");

  TASK *p_task = task_table;
  PROCESS *p_proc = proc_table;
  char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
  u16 selector_ldt = SELECTOR_LDT_FIRST;
  int i;
  for (i = 0; i < NR_TASKS; i++)
  {
    strcpy(p_proc->p_name, p_task->name); // name of the process
    p_proc->pid = i;                      // pid

    p_proc->sleep_time = 0;
    p_proc->wait = 0;

    p_proc->ldt_sel = selector_ldt;

    memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
           sizeof(DESCRIPTOR));
    p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
    memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
           sizeof(DESCRIPTOR));
    p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
    p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

    p_proc->regs.eip = (u32)p_task->initial_eip;
    p_proc->regs.esp = (u32)p_task_stack;
    p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

    p_task_stack -= p_task->stacksize;
    p_proc++;
    p_task++;
    selector_ldt += 1 << 3;
  }

  proc_table[0].ticks = proc_table[0].priority = 1;
  proc_table[1].ticks = proc_table[1].priority = 1;
  proc_table[2].ticks = proc_table[2].priority = 1;
  proc_table[3].ticks = proc_table[3].priority = 1;
  proc_table[4].ticks = proc_table[4].priority = 1;
  proc_table[5].ticks = proc_table[5].priority = 1;

  k_reenter = 0;
  ticks = 0;

  clearScreen();

  p_proc_ready = proc_table;

  /* 初始化信号量 */
  initSemaphore(&rmutex, 1);
  initSemaphore(&wmutex, 1);
  initSemaphore(&mutex, NUM_OF_READERS);
  initSemaphore(&semaphore, 1);
  readCount = 0;

  /* 初始化 8253 PIT */
  out_byte(TIMER_MODE, RATE_GENERATOR);
  out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
  out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

  put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
  enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

  // clearScreen();

  restart();

  while (1)
  {
  }
}

#ifdef READER
// 读者优先
/*======================================================================*
                               A
 *======================================================================*/
void A()
{
  while (1)
  {
    P(&mutex);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(20000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();
    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
    sleep(150000);
  }
}

/*======================================================================*
                               B
 *======================================================================*/
void B()
{
  while (1)
  {
    P(&mutex);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();
    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
    sleep(150000);
  }
}

/*======================================================================*
                               C
 *======================================================================*/
void C()
{
  while (1)
  {
    P(&mutex);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();

    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
    sleep(150000);
  }
}

/*======================================================================*
                               D
 *======================================================================*/
void D()
{
  while (1)
  {
    P(&wmutex);
    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts writing.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes writing.\n", 2);
    refreshScreen();
    V(&wmutex);
    sleep(150000);
  }
}

/*======================================================================*
                               E
 *======================================================================*/
void E()
{
  while (1)
  {
    P(&wmutex);
    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts writing.\n", 1);
    refreshScreen();
    milli_delay(40000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes writing.\n", 2);
    refreshScreen();
    V(&wmutex);
    sleep(150000);
  }
}

/*======================================================================*
                               F
 *======================================================================*/

void F()
{
  while (1)
  {
    if (readCount > 0)
    {
      print("Now reading tasks:");
      disp_int(readCount);
      print(".\n");
    }
    else if (readCount == 0)
    {
      if (wmutex.count <= 0)
      {
        print("Now writing.\n");
      }
      else
      {
        print("idle.\n");
      }
    }
    refreshScreen();
    milli_delay(10000);
  }
}

#else
// 写者优先
/*======================================================================*
                               A
 *======================================================================*/
void A()
{
  while (1)
  {
    P(&mutex);
    P(&semaphore);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&semaphore);
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(20000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();
    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
  }
}

/*======================================================================*
                               B
 *======================================================================*/
void B()
{
  while (1)
  {
    P(&mutex);
    P(&semaphore);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&semaphore);
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();
    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
  }
}

/*======================================================================*
                               C
 *======================================================================*/
void C()
{
  while (1)
  {
    P(&mutex);
    P(&semaphore);
    P(&rmutex);
    if (readCount == 0)
    {
      P(&wmutex);
    }
    readCount++;
    V(&semaphore);
    V(&rmutex);

    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts reading.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes reading.\n", 2);
    refreshScreen();

    P(&rmutex);
    readCount--;
    if (readCount == 0)
    {
      V(&wmutex);
    }
    V(&rmutex);
    V(&mutex);
  }
}

/*======================================================================*
                               D
 *======================================================================*/
void D()
{
  while (1)
  {
    P(&semaphore);
    P(&wmutex);
    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts writing.\n", 1);
    refreshScreen();
    milli_delay(30000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes writing.\n", 2);
    refreshScreen();
    V(&wmutex);
    V(&semaphore);
  }
}

/*======================================================================*
                               E
 *======================================================================*/
void E()
{
  while (1)
  {
    P(&semaphore);
    P(&wmutex);
    disp_color_str(p_proc_ready->p_name, 1);
    disp_color_str(" starts writing.\n", 1);
    refreshScreen();
    milli_delay(40000);
    disp_color_str(p_proc_ready->p_name, 2);
    disp_color_str(" finishes writing.\n", 2);
    refreshScreen();
    V(&wmutex);
    V(&semaphore);
  }
}

/*======================================================================*
                               F
 *======================================================================*/
void F()
{
  while (1)
  {
    if (readCount > 0)
    {
      print("Now reading tasks:");
      disp_int(readCount);
      print(".\n");
    }
    else if (readCount == 0)
    {
      if (wmutex.count <= 0)
      {
        print("Now writing.\n");
      }
      else
      {
        print("idle.\n");
      }
    }
    refreshScreen();
    milli_delay(10000);
  }
}
#endif