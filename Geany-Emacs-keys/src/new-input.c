#include "commands.h"

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

gint snoop()
{
	switch(in_state)
	{
	case STATE_CMD:
		if((cmd_in_cnt < 3 && cmd_in_cnt > 0 && cmd_in_need) || cmd_in_cnt == 0)
		{
			/*
			  modifiers set here
			*/

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
			else if(found == -1) //not found
			{
				input_reset();
				return FALSE;
			}
		}//if(cmd_in_cmd < 3 && ...
		break;

	case STATE_KEY:
		emacs_hotkey cur_key;
		cur_key.keyval = event->keyval;
		//k.alt = ...
		/* modifiers set here */

		cmd_param_data = malloc(sizeof(emacs_hotkey));
		*cmd_param_data = cur_key;

		func_call(cmd_current,cmd_repeat_cnt,cmd_param_data);
		input_reset();
		break;

	case STATE_INT:
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
}

