
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);
PRIVATE void search(CONSOLE *p_con, char *str, u8 color);
PUBLIC void clearConsole();

/* 搜索模式不同阶段, 0代表未进入搜索模式, 1为关键字输入阶段, 2为结果展示阶段*/
EXTERN int SEARCH_MODE;

PRIVATE u32 enterList[25] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
PRIVATE u32 *p_enter = &enterList[0];
PRIVATE u32 tabList[25] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
PRIVATE u32 *p_tab = &tabList[0];
PRIVATE char str[256];
PRIVATE char *p_str;
PRIVATE u8 str_tab[25] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
PRIVATE u8 *p_str_tab = &str_tab[0];
PRIVATE u32 cursorPos; /* 用于存储进入搜索模式前的cursor位置 */
PRIVATE struct s_console *p_con;
PRIVATE u8 strlength;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	p_con = p_tty->p_console;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		// out_char(p_tty->p_console, nr_tty + '0');
		// out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	if (SEARCH_MODE == 2)
	{
		if (ch == 0)
		{
			SEARCH_MODE = 0; // exit search mode
			for (int i = 0; i < p_con->cursor - cursorPos; i++)
			{
				*(p_vmem - 2) = ' ';
				*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
				p_vmem--;
				p_vmem--;
			}
			for (int i = 0; i < 25; i++)
			{
				if (tabList[i] > cursorPos && tabList[i] < cursorPos + strlength)
				{
					tabList[i] = -1;
				}
			}
			search(p_con, str, DEFAULT_CHAR_COLOR);
			p_con->cursor = cursorPos;
		}
	}
	else
	{
		switch (ch)
		{
		case 0:
			if (SEARCH_MODE == 0)
			{
				SEARCH_MODE = 1;
				strlength = 0;
				cursorPos = p_con->cursor;
				for (int i = 0; i < 256; i++)
				{
					str[i] = '\0';
				}
				p_str = &str[0];
				for (int i = 0; i < 25; i++)
				{
					str_tab[i] = -1;
				}
				p_str_tab = &str_tab[0];
			}
			else if (SEARCH_MODE == 1)
			{
				SEARCH_MODE = 0;
				for (int i = 0; i < p_con->cursor - cursorPos; i++)
				{
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					p_vmem--;
					p_vmem--;
				}
				p_con->cursor = cursorPos;
			}
			break;
		case '\n':
			if (SEARCH_MODE == 1)
			{
				SEARCH_MODE = 2;
				search(p_con, str, RED_CHAR_COLOR);
				break;
			}
			if (p_con->cursor < p_con->original_addr +
															p_con->v_mem_limit - SCREEN_WIDTH)
			{
				if (p_enter < &enterList[25])
				{
					*p_enter = p_con->cursor;
					p_enter++;
				}
				p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
																									 ((p_con->cursor - p_con->original_addr) /
																												SCREEN_WIDTH +
																										1);
			}
			break;
		case '\t':
			if (p_con->cursor <
					p_con->original_addr + p_con->v_mem_limit - 1)
			{
				if (p_tab < &tabList[25])
				{
					*p_tab = p_con->cursor;
					p_tab++;
				}
				if (SEARCH_MODE == 1)
				{
					*p_str_tab++ = p_con->cursor;
					strlength += 4;
				}
				for (int i = 0; i < 4; i++)
				{
					*p_vmem++ = ' ';
					*p_vmem++ = DEFAULT_CHAR_COLOR;
					p_con->cursor++;
					if (SEARCH_MODE == 1)
					{
						*p_str++ = ' ';
					}
				}
			}
			break;
		case '\b':

			if (p_con->cursor > p_con->original_addr)
			{
				if (SEARCH_MODE == 0)
				{
					if ((p_con->cursor - p_con->original_addr) % 80 == 0)
					{
						for (int i = 0; &enterList[i] <= p_enter; i++)
						{
							if (enterList[i] >= p_con->cursor - 80 && enterList[i] < p_con->cursor)
							{
								p_con->cursor = enterList[i];
								break;
							}
						}
					}
					else
					{
						short flag = 0;
						for (int i = 0; &tabList[i] <= p_tab; i++)
						{
							if (tabList[i] == p_con->cursor - 4)
							{
								p_con->cursor = tabList[i];
								tabList[i] = -1;
								p_tab = &tabList[i];
								flag = 1;
								break;
							}
						}
						if (!flag)
						{
							p_con->cursor--;
							*(p_vmem - 2) = ' ';
							*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
						}
					}
				}
				else if (SEARCH_MODE == 1 && p_con->cursor > cursorPos)
				{
					short flag = 0;
					for (int i = 0; &tabList[i] <= p_tab; i++)
					{
						if (tabList[i] == p_con->cursor - 4)
						{
							p_con->cursor = tabList[i];
							tabList[i] = -1;
							p_tab = &tabList[i];
							for (int j = 0; j < 4; j++)
							{
								--p_str;
								*p_str = '\0';
							}
							*--p_str_tab = -1;
							strlength -= 4;
							flag = 1;
							break;
						}
					}
					if (!flag)
					{
						p_con->cursor--;
						--p_str;
						*p_str = '\0';
						strlength--;
						*(p_vmem - 2) = ' ';
						*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
					}
				}
			}
			break;
		default:
			if (p_con->cursor <
					p_con->original_addr + p_con->v_mem_limit - 1)
			{
				*p_vmem++ = ch;
				if (SEARCH_MODE == 0)
				{
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
				else
				{
					*p_vmem++ = RED_CHAR_COLOR;
					*p_str++ = ch;
					strlength++;
				}
				p_con->cursor++;
			}
			break;
		}
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	if (is_current_console(p_con))
	{
		set_cursor(p_con->cursor);
		set_video_start_addr(p_con->current_start_addr);
	}
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	flush(&console_table[nr_console]);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
				p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}

	flush(p_con);
}

void search(CONSOLE *p_con, char *str, u8 color)
{
	u8 *p_vmem_start = (u8 *)(V_MEM_BASE + p_con->original_addr * 2);
	u8 *p_vmem_end = (u8 *)(V_MEM_BASE + cursorPos * 2);
	u8 *tmp;
	u32 count = p_con->original_addr;
	u8 flag;

	/* debug */
	// *(p_vmem_start + 160) = '0' + (u8)tabList[0] - count;
	// *(p_vmem_start + 162) = '0' + (u8)tabList[1] - count;
	// *(p_vmem_start + 164) = '0' + (u8)tabList[2] - count;
	// *(p_vmem_start + 166) = '0' + (u8)tabList[3] - count;
	// *(p_vmem_start + 168) = '0' + (u8)tabList[4] - count;

	// *(p_vmem_start + 170) = '0' + (u8)str_tab[0] - cursorPos;
	// *(p_vmem_start + 172) = '0' + (u8)str_tab[1] - cursorPos;
	// *(p_vmem_start + 174) = '0' + (u8)str_tab[2] - cursorPos;
	// *(p_vmem_start + 176) = '0' + (u8)str_tab[3] - cursorPos;
	// *(p_vmem_start + 178) = '0' + (u8)str_tab[4] - cursorPos;

	while (p_vmem_start < p_vmem_end)
	{
		tmp = p_vmem_start;
		flag = 1;
		/* 在忽略空格和tab的区别的前提下进行字符串比较 */
		for (int i = 0; i < 256 && str[i] != '\0'; i++)
		{
			if (*tmp != str[i])
			{
				flag = 0;
				break;
			}
			tmp++;
			tmp++;
		}
		if (flag)
		{
			/* 用于比较字符串中的tab */
			for (int i = 0; &str_tab[i] < p_str_tab && str_tab[i] != -1; i++)
			{
				if (flag == 0)
				{
					break;
				}
				u8 flag2 = 0;
				for (int j = 0; &tabList[j] < p_tab && tabList[j] != -1; j++)
				{
					u8 offset_1 = tabList[j] - count;
					u8 offset_2 = str_tab[i] - cursorPos;
					if (offset_1 == offset_2)
					{
						flag2 = 1;
						break;
					}
				}
				flag = flag2;
			}
			for (int i = 0; &tabList[i] < p_tab && tabList[i] != -1; i++)
			{
				if (tabList[i] < count)
				{
					continue;
				}
				u8 offset_1 = tabList[i] - count;
				if (offset_1 >= strlength)
				{
					break;
				}
				if (flag == 0)
				{
					break;
				}
				u8 flag2 = 0;
				for (int j = 0; &str_tab[j] < p_str_tab && str_tab[j] != -1; j++)
				{
					u8 offset_2 = str_tab[j] - cursorPos;
					if (offset_1 == offset_2)
					{
						flag2 = 1;
						break;
					}
				}
				flag = flag2;
			}

			if (flag)
			{
				/* 修改颜色 */
				tmp = p_vmem_start;
				for (int i = 0; i < 256 && str[i] != '\0'; i++)
				{
					tmp++;
					*tmp++ = color;
				}
			}
		}
		count++;
		p_vmem_start++;
		p_vmem_start++;
	}
	flush(p_con);
}

void clearConsole()
{
	if (SEARCH_MODE != 0)
	{
		return;
	}
	u8 *p_vmem;
	while (p_con->cursor > p_con->original_addr)
	{
		p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
		p_con->cursor--;
		*(p_vmem - 2) = ' ';
		*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
	}
	flush(p_con);
}