#ifndef COMMANDS_H
#define COMMANDS_H

#include <geanyplugin.h>

#define COMMANDS_NUM 3

typedef void (*command_func)(void* data);

enum _command_func_param_type
{
	PT_NONE = 0,
	PT_NUM  = 1,
	PT_STR  = 2,
	PT_CMD  = 3
};

enum _keyval_type
{
	KT_NONE = 0,
	KT_SINGLE = 1,
	KT_REPEAT = 2,
	KT_RANGE_NUMBERS  = 3,
	KT_RANGE_CHARS = 4
};

typedef struct _emacs_hotkey
{
	gboolean ctrl;
	gboolean alt;
	gboolean shift;
	guint keyval_type;
	guint keyval;
}emacs_hotkey;

typedef struct _emacs_keybinding
{
	emacs_hotkey keys[3];
	guint keys_num;
	command_func func;
	guint param_type;
}emacs_keybinding;


/* keybindings implementation  */
//ctrl,alt,shift,key, type
emacs_keybinding commands[COMMANDS_NUM] =
{
	{ //C-k, kill_line
		{ {TRUE,FALSE,FALSE,KT_SINGLE,GDK_k},
		  {0,0,0,0,0},
		  {0,0,0,0,0}
		},
		1,
		kill_line,
		PT_NONE
	},
	{ //M-<num>, set_repeat
		{ {FALSE,TRUE,FALSE,KT_RANGE_NUMBERS},
		  {0,0,0,0,0},
		  {0,0,0,0,0}
		},
		1,
		set_repeat,
		PT_NUM
	},
	{ //C-x z(z), repeat
		{ {TRUE,FALSE,FALSE,KT_SINGLE,GDK_x},
		  {FALSE,FALSE,FALSE,KT_REPEAT,GDK_z},
		  {0,0,0,0,0}
		},
		2,
		repeat,
		PT_NONE
	}
};







