//      keymap.c
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

#include <geanyplugin.h>
#include "global.h"

//TODO: make it work
void quit(void)
{
	//g_signal_emit_by_name(geany_data->main_widgets->window,"delete_event");
	msgwin_msg_add(COLOR_RED,-1,NULL,"Emacs keys plugin: Not implemented yet (C-x C-c)");
}

void save_buffer(void)
{
	keybindings_send_command(GEANY_KEY_GROUP_FILE,GEANY_KEYS_FILE_SAVE);
}

void kill_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	guint cur_line = sci_get_current_line(editor);
	guint line_beg = sci_get_position_from_line(editor,cur_line);

	gint start_pos = sci_get_current_position(editor);
	gint end_pos = (sci_get_line_length(editor,cur_line) - (start_pos - line_beg)) + (start_pos-1);

	scintilla_send_message(editor,SCI_COPYRANGE,start_pos,end_pos);
	scintilla_send_message(editor,SCI_DELLINERIGHT,0,0);
}

void backward_char(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_CHARLEFT,0,0);
}

void forward_char(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_CHARRIGHT,0,0);
}

void previous_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_LINEUP,0,0);
}

void next_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_LINEDOWN,0,0);
}

void forward_word(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_WORDRIGHT,0,0);
}

void backward_word(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_WORDLEFT,0,0);
}

void beginning_of_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_HOME,0,0);
}

void end_of_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_LINEEND,0,0);
}

void undo(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_UNDO,0,0);
}

void find_file(void)
{
	keybindings_send_command(GEANY_KEY_GROUP_FILE,GEANY_KEYS_FILE_OPEN);
}

void open_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	guint pos = sci_get_current_position(editor);
	scintilla_send_message(editor,SCI_NEWLINE,0,0);
	sci_set_current_position(editor,pos,FALSE);
}

void kill_whole_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_LINECUT,0,0);
}

void beginning_of_buffer(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_DOCUMENTSTART,0,0);
}

void end_of_buffer(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	scintilla_send_message(editor,SCI_DOCUMENTEND,0,0);
}

void goto_line(void)
{
	GeanyDocument* doc = document_get_current();
	ScintillaObject* editor = doc->editor->sci;

	keybindings_send_command(GEANY_KEY_GROUP_GOTO,GEANY_KEYS_GOTO_LINE);
}
