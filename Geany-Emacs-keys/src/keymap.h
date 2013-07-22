#ifndef KEYMAP_H
#define KEYMAP_H

#include <geanyplugin.h>

//number of implemented keybindings
#define SHORTCUTS_NUM 18

typedef void (*on_key)(void);

typedef struct shortcut_t
{
	gboolean alt; 	//is alt(MOD1) key pressed
	gboolean ctrl;
	gboolean shift;
	guint keyval;
}shortcut;

void quit(void);                //C-x C-c
void save_buffer(void);         //C-x C-s
void kill_line(void);           //C-k
void backward_char(void);       //C-b
void forward_char(void);        //C-f
void previous_line(void);       //C-p
void next_line(void); 	        //C-n
void forward_word(void);        //M-f
void backward_word(void);       //M-b
void beginning_of_line(void);   //C-a
void end_of_line(void);         //C-e
void undo(void);                //C-/
void find_file(void);           //C-x C-f
void open_line(void);           //C-o
void kill_whole_line(void);     //C-S-Bakspace
void beginning_of_buffer(void); //M-<
void end_of_buffer(void);       //M->
void goto_line(void);           //M-g

const on_key keybindings_funcs[SHORTCUTS_NUM] = {
	save_buffer,        //1
	quit,               //2
	kill_line,          //3
	backward_char,      //4
	forward_char,       //5
	previous_line,      //6
	next_line,          //7
	forward_word,       //8
	backward_word,      //9
	beginning_of_line,  //10
	end_of_line,        //11
	undo,               //12
	find_file,          //13
	open_line,          //14
	kill_whole_line,    //15
	beginning_of_buffer,//16
	end_of_buffer,      //17
	goto_line           //18
};

// {alt,ctrl,shift,keyval}
const shortcut keybindings[SHORTCUTS_NUM][3] = {
		{ {0,1,0,GDK_x        }, { 0,1,0,GDK_s}, {0,0,0,0} }, //1
		{ {0,1,0,GDK_x        }, { 0,1,0,GDK_c}, {0,0,0,0} }, //2
		{ {0,1,0,GDK_k        }, { 0,0,0,0    }, {0,0,0,0} }, //3
		{ {0,1,0,GDK_b        }, { 0,0,0,0    }, {0,0,0,0} }, //4
		{ {0,1,0,GDK_f        }, { 0,0,0,0    }, {0,0,0,0} }, //5
		{ {0,1,0,GDK_p        }, { 0,0,0,0    }, {0,0,0,0} }, //6
		{ {0,1,0,GDK_n        }, { 0,0,0,0    }, {0,0,0,0} }, //7
		{ {1,0,0,GDK_f        }, { 0,0,0,0    }, {0,0,0,0} }, //8
		{ {1,0,0,GDK_b        }, { 0,0,0,0    }, {0,0,0,0} }, //9
		{ {0,1,0,GDK_a        }, { 0,0,0,0    }, {0,0,0,0} }, //10
		{ {0,1,0,GDK_e        }, { 0,0,0,0    }, {0,0,0,0} }, //11
		{ {0,1,0,GDK_slash    }, { 0,0,0,0    }, {0,0,0,0} }, //12
		{ {0,1,0,GDK_x        }, { 0,1,0,GDK_f}, {0,0,0,0} }, //13
		{ {0,1,0,GDK_o        }, { 0,0,0,0    }, {0,0,0,0} }, //14
		{ {0,1,1,GDK_BackSpace}, { 0,0,0,0    }, {0,0,0,0} }, //15
		{ {1,0,1,GDK_less     }, { 0,0,0,0    }, {0,0,0,0} }, //16
		{ {1,0,1,GDK_greater  }, { 0,0,0,0    }, {0,0,0,0} }, //17
		{ {1,0,0,GDK_g        }, { 0,0,0,0    }, {0,0,0,0} }  //18
	};

#endif

