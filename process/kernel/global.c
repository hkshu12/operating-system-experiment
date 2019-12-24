
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"

PUBLIC PROCESS proc_table[NR_TASKS];

PUBLIC char task_stack[STACK_SIZE_TOTAL];

PUBLIC TASK task_table[NR_TASKS] = {{A, STACK_SIZE_A, "A"},
                                    {B, STACK_SIZE_B, "B"},
                                    {C, STACK_SIZE_C, "C"},
                                    {D, STACK_SIZE_D, "D"},
                                    {E, STACK_SIZE_E, "E"},
                                    {F, STACK_SIZE_F, "F"}};

PUBLIC irq_handler irq_table[NR_IRQ];

PUBLIC system_call sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_print, sys_color_print, sys_sleep, sys_p, sys_v};

PUBLIC void clearScreen()
{
  disp_pos = 0;
  for (int i = 0; i < 80 * 25; i++)
  {
    disp_str(" ");
  }
  disp_pos = 0;
}

PUBLIC void refreshScreen()
{
  if (disp_pos > 25 * 80 * 2)
  {
    clearScreen();
  }
}