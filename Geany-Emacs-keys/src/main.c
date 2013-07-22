//      main.c
//
//      Copyright 2010 Alexey Denisov <xelxelxelxel@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.


#include <stdio.h>
#include <string.h>
#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include "commands.h"
#include "global.h"

PLUGIN_VERSION_CHECK(147)
PLUGIN_SET_INFO("Emacs keys", "An implementation of Emacs keybindings",
                "0.2a", "Alexey Denisov <xelxelxelxel@gmail.com");

static gint SNOOPER_ID;

enum input_state
{
	STATE_CMD = 3,
	STATE_KEY = 4,
	STATE_INT = 1,
	STATE_STR = 2
};

guint in_state;

void* param_data;
int param_in_cnt;

int cmd_in_cnt;
gboolean cmd_in_need;
emacs_hotkey cmd_in_buf[3];
emacs_keybinding* cmd_current;

emacs_keybinding* cmd_repeat;
guint cmd_repeat_cnt;

static const MAX_PARAM_STR_LEN = 512;

static enum _keyval_type get_range(guint keyval)
{
	guint num_start = gdk_unicode_to_keyval('0');
	guint num_end = gdk_unicode_to_keyval('9');

	guint chr_start = gdk_unicode_to_keyval('a');
	guint chr_end = gdk_unicode_to_keyval('z');

	keyval = gdk_keyval_to_lower(keyval);

	if(keyval >= num_start && keyval <= num_end)
		return KT_RANGE_NUMBERS;

	if(keyval >= chr_start && keyval <= chr_end)
		return KT_RANGE_CHARS;

	//keyval isn't in the range
	return KT_SINGLE;
}

/*
 Compare two hotkeys

 Return value:
  0 - k1 != k2
  1 - k1 == k2
  2 - equal types, but keyvals */
 static guint keycmp(emacs_hotkey k1,emacs_hotkey k2)
{
	if(k1.alt != k2.alt)
		return 0;
	if(k1.shift != k2.shift)
		return 0;
	if(k1.ctrl != k2.ctrl)
		return 0;

	guint range;

	if(k1.keyval_type == KT_NONE)
	{
		switch(k2.keyval_type)
		{
		case KT_NONE:
		case KT_SINGLE:
		case KT_REPEAT:
			if(k1.keyval != k2.keyval)
				return 0;
			break;
		case KT_RANGE_NUMBERS:
		case KT_RANGE_CHARS:
			range = get_range(k1.keyval);

			if(range != k2.keyval_type)
				return 0;
			else
				if(k1.keyval != k2.keyval)
					return 2;

			break;
		}
	}
	else if(k1.keyval_type == KT_SINGLE || k1.keyval_type == KT_REPEAT)
	{
		if(k1.keyval_type != k2.keyval_type ||
		   k2.keyval != k1.keyval)
			return 0;
	}
	else //KT_RANGE_CHARS, KT_RANGE_NUMBERS
	{
		switch(k2.keyval_type)
		{
		case KT_RANGE_NUMBERS:
		case KT_RANGE_CHARS:
			if(k1.keyval_type == k2.keyval_type) {
				if(k1.keyval != k2.keyval)
					return 2;
			}
			else
				return 0;

			break;
		case KT_SINGLE:
		case KT_REPEAT:
			if(k1.keyval != k2.keyval ||
				k2.keyval_type != k2.keyval_type)
				return 0;
			break;
		}
	}

	return 1;
}

static gint keybinding_find(emacs_hotkey keys_seq[3])
{
	int kb_i;
	for(kb_i=0;kb_i<SHORTCUTS_NUM;kb_i++)
	{
		int ks_i;
		for(ks_i=0;ks_i<3;ks_i++)
		{
			if(keycmp(keys_seq[ks_i],keybindings[kb_i][ks_i])==0)
				break;
		}

		if(ks_i == 3) //found
			return kb_i;

		if(ks_i == 0 && kb_i==SHORTCUTS_NUM-1) //not found
			return -1;

		if(ks_i>0)
		{
			if(keybindings[kb_i][ks_i].keyval!=0 && keys_seq[ks_i].keyval==0) //partially found
				return -2;
		}
	}

	return -1; //not found
}

static void input_reset(void)
{
	cmd_in_need = 0;
	memset(&cmd_in_buf,0,sizeof(shortcut)*3);
	cmd_in_cnt=0;
}

static void func_call(emacs_keybinding* cmd, guint repeat,void* param)
{
	gint i;
	for(i=0;i<repeat;i++)
		(*cmd->func)(param);
}

static int keyval2int(guint keyval)
{
	//FIXME: i think that this is bad
	guint32 chr[2];
	char* end;

	chr[0] = gdk_keyval_to_unicode(keyval);
	chr[1] = 0;

	//FIXME: error handling
	return strtol((char*) chr,&end,10);
}

static char keyval2char(guint keyval)
{
	//FIXME: do something with char and
	//guint32
	return (char) gdk_keyval_to_unicode(keyval);
}

static gint key_snooper(GtkWidget* widget,GdkEventKey* key_event,gpointer data)
{
	if(key_event->type == GDK_KEY_PRESS &&
	   widget == geany_data->main_widgets->window)
	{
		if(key_event->keyval == GDK_Shift_L) {
			cmd_in_buf[cmd_in_cnt].shift = TRUE;
			return TRUE;
		}
		if(key_event->keyval == GDK_Control_L) {
			cmd_in_buf[cmd_in_cnt].ctrl = TRUE;
			return TRUE;
		}
		if(key_event->keyval == GDK_Alt_L) {
			cmd_in_buf[cmd_in_cnt].alt = TRUE;
			return TRUE;
		}

		switch(in_state)
		{
		case STATE_CMD:
			if((cmd_in_cnt < 3 && cmd_in_cnt > 0 && cmd_in_need) || cmd_in_cnt == 0)
			{
				if(key_event->state & GDK_SHIFT_MASK)
					cmd_in_buf[cmd_in_cnt].shift = TRUE;
				if(key_event->state & GDK_CONTROL_MASK)
					cmd_in_buf[cmd_in_cnt].ctrl = TRUE;
				if(key_event->state & GDK_MOD1_MASK)
					cmd_in_buf[cmd_in_cnt].alt = TRUE;

				cmd_in_buf[cmd_in_cnt].keyval = key_event->keyval;
				cmd_in_buf[cmd_in_cnt].keyval_type = KT_SINGLE;

				int found;
				found = shortcut_find(cmd_in_buf);

				cmd_in_cnt++;

				if(found >= 0)
				{
					/* this is not parameter */
					if(!cmd_current)
					{
						/* function does not have any parameters */
						if(commands[found]->param_type==PT_NONE)
						{
							func_call(commands[found],cmd_repeat_cnt,NULL);
						}
						/* this cmd is parameter for function */
						else
						{
							in_state = commands[found]->param_type;
							cmd_current = commands[found];
						}
					}
					/* this is a parameter */
					else
					{
						cmd_param_data = malloc(sizeof(emacs_keybinding));
						*cmd_param_data = commands[found];

						func_call(commands[found],cmd_repeat_cnt,cmd_param_data);
					}

					input_reset();
					return TRUE;
				}
				else if(found == -2) //maybe found
				{
					cmd_in_need = TRUE;
					return TRUE;
				}
				else if(found == -1)
				{
					input_reset();
					return FALSE;
				}
			}//if(cmd_in_cmd < 3 && ...
			break;

		case STATE_KEY:
			emacs_hotkey cur_key;
			cur_key.keyval = event->keyval;

			if(key_event->state & GDK_SHIFT_MASK)
				cur_key.shift = TRUE;
			if(key_event->state & GDK_CONTROL_MASK)
				cur_key.ctrl = TRUE;
			if(key_event->state & GDK_MOD1_MASK)
				cur_key.alt = TRUE;

			cmd_param_data = malloc(sizeof(emacs_hotkey));
			*cmd_param_data = cur_key;

			func_call(cmd_current,cmd_repeat_cnt,cmd_param_data);
			input_reset();
			break;

		case STATE_INT:

			if( get_range(key_event->keyval) != KT_RANGE_NUMBERS)
			{

			}


			if(!cmd_param_data)
			{
				cmd_param_data = malloc(sizeof(int));
				*cmd_param_data = 0;
			}

			param_in_cnt++;

			gint data = *cmd_param_data;
			data  = keyval2int(event->keyval)*pow(param_in_cnt,10) + data;
			*cmd_param_data = data;

			break;

		case STATE_STR:
			if(!cmd_param_data)
			{
				cmd_param_data  = malloc(sizeof(gchar)*PARAM_STR_MAX);
			}

			param_in_cnt++;

			gchar data = cmd_param_data[param_in_cnt];
			data = keyval2char(event->keyval);
			cmd_param_data[param_in_cnt];
			break;
	}

	return FALSE;
}

void plugin_init(GeanyData *data)
{
	input_reset();
	input_need = FALSE;
	repeat = NULL;
	repeat_count = 1;

	SNOOPER_ID = gtk_key_snooper_install(key_snooper,NULL);
}

void plugin_cleanup(void)
{
	gtk_key_snooper_remove(SNOOPER_ID);
}




