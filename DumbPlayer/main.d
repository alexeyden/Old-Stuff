module DrugPlayer;

import gtk.Window;
import gtk.Widget;
import gtk.Label;
import gtk.HScale;
import gtk.Range;
import gtk.ToolButton;
import gtk.MenuToolButton;
import gtk.Main;
import gtk.Builder;
import gtk.ListStore;
import gtk.TreeModel;
import gtk.TreePath;
import gtk.TreeIter;
import gtk.TreeView;
import gtk.TreeViewColumn;
import gtk.TreeModelIF;
import gtk.Image;
import gtk.RadioMenuItem;
import gtk.MenuItem;
import gtk.PopupBox;
import gtk.CellRendererText;
import gtk.CellRendererToggle;
import gtk.SelectionData;
import gtk.DragAndDrop;
import gobject.Value;
import gdk.Event;
import gdk.Gdk;

import gdk.DragContext;
import gtkc.gtktypes;
import gtkc.gdktypes;

import glib.Str;
import glib.Timeout;
import gtkc.glibtypes;

import gstreamer.gstreamer;
import gstreamer.Element;
import gstreamer.ElementFactory;
import gstreamer.Message;
import gstreamer.Format;
import gstreamerc.gstreamertypes;

import std.path;
//import std.c.stdio;
import std.stdio;
import std.file;
import std.string;
import std.uri;
import std.stdio;
import std.array;

import player;
import config;

class DPlayer: Player
{
	private:
		Window main_win;
		ToolButton toggle_play;
		ToolButton btn_stop;
		ToolButton btn_prev;
		MenuToolButton btn_next;
		RadioMenuItem item_linear;
		RadioMenuItem item_random;
		RadioMenuItem item_repeat;
		Label time;
		HScale seek;
		Builder builder;
		TreeView list;
		Label remove_area;
		Timeout label_updater;

		bool is_seeking;
		
		const DRAG_DEST_ID = 80;
		const PLAYLIST_DRAG_ID = 42;
		
		void setup_gui(TreeModelIF model,string ui_file)
		{
			builder = new Builder();
			if(!builder.addFromFile(ui_file))
			{
				stderr.writeln("Cannot open UI file!");
				Main.quit();
			}
			
			main_win = cast(Window) builder.getObject("MainWin");
			
			toggle_play = cast(ToolButton) builder.getObject("btn_play");		
			btn_stop = cast(ToolButton) builder.getObject("btn_stop");
			btn_next = cast(MenuToolButton) builder.getObject("btn_next");
			btn_prev = cast(ToolButton) builder.getObject("btn_prev");
			
			seek = cast(HScale) builder.getObject("progress");
			time = cast(Label) builder.getObject("time");
			
			remove_area = cast(Label) builder.getObject("remove");
			list = cast(TreeView) builder.getObject("playlist");
			
			item_linear = cast(RadioMenuItem) builder.getObject("linear");
			item_random = cast(RadioMenuItem) builder.getObject("random");
			item_repeat = cast(RadioMenuItem) builder.getObject("repeat");
				
			/* playlist columns */
			auto toggle_renderer = new CellRendererToggle();
			toggle_renderer.setRadio(true);
			
			list.setModel(model);
			TreeViewColumn file_col =
				new TreeViewColumn("File",new CellRendererText(),"text",1);
			TreeViewColumn status_col =
				new TreeViewColumn(" ",toggle_renderer,"active",0);
			list.appendColumn(status_col);
			list.appendColumn(file_col);
			
			file_col.setResizable(true);
			file_col.setReorderable(true);
			file_col.setSortColumnId(0);
			file_col.setSortIndicator(true);
		}
		
		void setup_dragdrop()
		{
			GtkTargetEntry[1] uri_te = [
				GtkTargetEntry( cast(char*)"text/uri-list", 0, DRAG_DEST_ID)
			];
			GtkTargetEntry playlist_te = 
				GtkTargetEntry( cast(char*)"playlist-item", 0, PLAYLIST_DRAG_ID);
			
			list.enableModelDragDest(
				uri_te,
				DragAction.ACTION_COPY | DragAction.ACTION_MOVE
			);
			list.enableModelDragSource(
				GdkModifierType.BUTTON1_MASK,
				[playlist_te],
				DragAction.ACTION_COPY | DragAction.ACTION_MOVE
			);
			list.addOnDragDataReceived(&this.onDropData);
			list.addOnDragBegin(&this.onDragStart);
			list.addOnDragEnd(&this.onDragStop);
			remove_area.addOnDragDrop(&this.onDragDropRemArea);
			DragAndDrop.destSet(
				cast(Widget) this.remove_area,
				GtkDestDefaults.ALL,
				[playlist_te],
				DragAction.ACTION_COPY | DragAction.ACTION_MOVE
			);
		}
	
		void onWinDestroy(Widget w) { Main.quit(); }
		void onButtonStop(ToolButton w)
		{
			stop(); 
			toggle_play.setIconName("gtk-media-play-ltr");
		}
		
		void onButtonNext(ToolButton w)
		{
			next();
		}
		void onButtonPrev(ToolButton w) { prev();}
		void onButtonPlay(ToolButton w)
		{	
			if(status == PlayStatus.STOPPED)
				onPlaySelected(null,null,null);
			else
			{
				toggle();
				
				if(status == PlayStatus.PAUSED) 
					toggle_play.setIconName("gtk-media-play-ltr");
				else
					toggle_play.setIconName("gtk-media-pause");
			}
		}
		void onDragStart(DragContext dc,Widget w) { remove_area.show(); }
		void onDragStop(DragContext dc,Widget w) { remove_area.hide(); }
		void onPlaySelected(TreePath p, TreeViewColumn c, TreeView v)
		{
			TreeIter it = list.getSelection().getSelected();
			if(it)
			{
				play(it);
				toggle_play.setIconName("gtk-media-pause");
			}
		}
		
		void
		onDropData(DragContext dc, int x, int y,SelectionData data, uint info, uint time, Widget w)
		{
			if (info == DRAG_DEST_ID)
			{
				string dat = Str.toString(data.dataGetText());
				replace(dat,"\r\n","");
				string[] splitted = split(dat);
				
				foreach(string s; splitted)
				{
					add(replace(decode(s[7..s.length]),"\r\n\0",""));
				}
			}
		}
		
		bool onDragDropRemArea(DragContext dc, int x, int y,uint time, Widget w)
		{
			//DragAndDrop context_dd = new DragAndDrop(dc);
			//context_dd.finish(1,1,time);

			return true;
		}
		
		bool onSeekingStart(Event ev, Widget w)
		{	
			is_seeking = true;
			
			return false;
		}
		bool onSeekingStop(Event ev, Widget w)
		{
			is_seeking = false;
			
			setPosition(cast(uint) seek.getValue());
			return false;
		}
		
		void onModeLinear(MenuItem item) { setMode(PlayMode.LINEAR); }
		void onModeRepeat(MenuItem item) { setMode(PlayMode.SINGLE); }
		void onModeRandom(MenuItem item) { setMode(PlayMode.RANDOM); }
		
		bool updatePosition()
		{
			if(!is_seeking)
			{
				long len = getDuration();
				long pos = getPosition();

				time.setText(format(
					(pos/60 < 10) ? "0" : "", pos/60, ':',
					(pos%60 < 10) ? "0" : "", pos%60, " / ",
					(len/60 < 10) ? "0" : "", len/60, ':',
					(len%60 < 10) ? "0" : "",len%60)
				);
				seek.setValue(pos);
			}
			return true;
		}
		
		protected override
		{
			void onDurationChange()
			{
				seek.setRange(0.0,cast(double)getDuration());
			}
			void onError()
			{
				toggle_play.setIconName("gtk-media-play-ltr");
				PopupBox.error("Something bad happened!","Error");
			}
		}
		
	public:
		this(string[] args,string ui_file)
		{
			super();
			
			if(args.length > 1)
			{
				for(int i=1;i<args.length;i++)
				{
					if(args[i][0]!='-')
						add(args[i]);
				}
			}
			
			setup_gui(playlist,ui_file);
			setup_dragdrop();
			
			main_win.addOnDestroy(&this.onWinDestroy);
			
			//buttons
			toggle_play.addOnClicked(&this.onButtonPlay);
			btn_stop.addOnClicked(&this.onButtonStop);
			btn_prev.addOnClicked(&this.onButtonPrev);
			btn_next.addOnClicked(&this.onButtonNext);

			//on play
			list.addOnRowActivated(&this.onPlaySelected);

			//seeking
			//seek.addOnButtonRelease(&this.onSeekingStop);
			//seek.addOnButtonPress(&this.onSeekingStart);
			
			//playing mode switch
			item_linear.addOnActivate(&this.onModeLinear);
			item_repeat.addOnActivate(&this.onModeRepeat);
			item_random.addOnActivate(&this.onModeRandom);

			label_updater = new Timeout(&this.updatePosition,1,GPriority.DEFAULT,false);

			//set window icon
			if(exists(DATA_DIR~"/scalable.svg"))
				main_win.setIconFromFile(DATA_DIR~"/scalable.svg");
			else
				main_win.setIconName("dumbplayer");
			
			is_seeking = false;
			
			main_win.show();
		}
}

int main(string[] args)
{
	foreach(s;args)
	{
		if(s == "--help" || s == "-h")
		{
			writeln("Usage: dumbplayer <FILES>");
			return 0;
		}
	}

	Main.init(args);
	GStreamer.init(args);
	
	string ui_file = DATA_DIR~"/gui.xml";

	if(!exists(ui_file))
	{
		writeln("Can't find UI file.");
		return 1;
	}
	
	DPlayer app = new DPlayer(args,ui_file);	
	
	Main.run();
	return 0;
}
