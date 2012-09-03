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
import gtk.ObjectGtk;
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
import std.c.stdio;
import std.file;
import std.string;
import std.uri;
import std.stdio;

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
			//load UI file
			builder = new Builder();
			int status = builder.addFromFile(ui_file);
			if(!status)
				Main.quit();
				
			//set UI widgets
			auto obj = builder.getObject("MainWin"); 
			obj.setData("GObject", null); 
			main_win = new Window(cast(GtkWindow*)obj.getObjectGStruct());
			
			
			/* playing control buttons */
			obj = builder.getObject("btn_play"); 
			obj.setData("GObject", null); 
			toggle_play = new ToolButton(cast(GtkToolButton*)obj.getObjectGStruct());
			
			obj = builder.getObject("btn_stop"); 
			obj.setData("GObject", null); 
			btn_stop = new ToolButton(cast(GtkToolButton*)obj.getObjectGStruct());
			
			obj = builder.getObject("btn_next"); 
			obj.setData("GObject", null); 
			btn_next = new MenuToolButton(cast(GtkMenuToolButton*)obj.getObjectGStruct());
			
			obj = builder.getObject("btn_prev"); 
			obj.setData("GObject", null); 
			btn_prev = new ToolButton(cast(GtkToolButton*)obj.getObjectGStruct());
			
			
			/* time widgets */
			obj = builder.getObject("progress"); 
			obj.setData("GObject", null); 
			seek = new HScale(cast(GtkHScale*)obj.getObjectGStruct());
			
			obj = builder.getObject("time"); 
			obj.setData("GObject", null); 
			time = new Label(cast(GtkLabel*)obj.getObjectGStruct());
			
			
			/* playlist and remove area widgets */
			obj = builder.getObject("remove"); 
			obj.setData("GObject", null); 
			remove_area = new Label(cast(GtkLabel*)obj.getObjectGStruct());
			
			obj = builder.getObject("playlist"); 
			obj.setData("GObject", null); 
			list = new TreeView(cast(GtkTreeView*)obj.getObjectGStruct());
			
			
			/* playlist mode menu items */
			obj = builder.getObject("linear"); 
			obj.setData("GObject", null); 
			item_linear = new RadioMenuItem(cast(GtkRadioMenuItem*)obj.getObjectGStruct());
			
			obj = builder.getObject("random"); 
			obj.setData("GObject", null); 
			item_random = new RadioMenuItem(cast(GtkRadioMenuItem*)obj.getObjectGStruct());
			
			obj = builder.getObject("repeat"); 
			obj.setData("GObject", null); 
			item_repeat = new RadioMenuItem(cast(GtkRadioMenuItem*)obj.getObjectGStruct());
				
				
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
				&playlist_te,
				1,
				DragAction.ACTION_COPY | DragAction.ACTION_MOVE
			);
		}
	
		void onWinDestroy(ObjectGtk w) { Main.quit(); }
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
		void onDragStart(GdkDragContext* dc,Widget w) { remove_area.show(); }
		void onDragStop(GdkDragContext* dc,Widget w) { remove_area.hide(); }
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
		onDropData(GdkDragContext* dc, int x, int y,
					GtkSelectionData* data, uint info, uint time, Widget w)
		{
			if (info == DRAG_DEST_ID)
			{
				string dat = Str.toString(data.data);
				replace(dat,"\r\n","");
				string[] splitted = split(dat);
				
				foreach(string s; splitted)
				{
					add(replace(decode(s[7..s.length]),"\r\n\0",""));
				}
			}
		}
		
		bool onDragDropRemArea(GdkDragContext* dc, int x, int y,uint time, Widget w)
		{
			DragAndDrop context_dd = new DragAndDrop(dc);
			context_dd.finish(1,1,time);

			return true;
		}
		
		bool onSeekingStart(GdkEventButton* ev, Widget w)
		{	
			is_seeking = true;
			
			return false;
		}
		bool onSeekingStop(GdkEventButton* ev, Widget w)
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
			seek.addOnButtonRelease(&this.onSeekingStop);
			seek.addOnButtonPress(&this.onSeekingStart);
			
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
	Main.init(args);
	GStreamer.init(args);
	
	foreach(s;args)
	{
		if(s == "--help" || s == "-h")
		{
			writeln("Usage: dumbplayer <FILES>");
			return 0;
		}
	}
	
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
