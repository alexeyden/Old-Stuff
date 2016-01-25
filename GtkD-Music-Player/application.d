module dumbplayer;

/* Gtk */
import gtk.Window, gtk.Widget, gtk.Label, gtk.Scale, gtk.Range;
import gtk.Main, gtk.Builder;
import gtk.ListStore, gtk.TreeModel, gtk.TreePath, gtk.TreeIter, gtk.TreeView;
import gtk.TreeViewColumn, gtk.TreeModelIF, gtk.TreeRowReference;
//import gtk.CellRendererText, gtk.CellRendererPixbuf;
import gtk.InfoBar;
import gtk.RadioMenuItem, gtk.MenuItem, gtk.Menu;
import gtk.SelectionData, gtk.DragAndDrop;
import gtk.ToolButton, gtk.MenuToolButton;

/* Gdk and GObject */
import gdk.Event, gdk.Gdk, gdk.DragContext;
import gobject.Value;

/* GLib */
import glib.Str;
import glib.Timeout;
import glib.ErrorG;

/* gtkc */
import gtkc.gtktypes;
import gtkc.gdktypes;
import gtkc.glibtypes;
import gtkc.glib;

/* gstreamer */
import gstreamerc.gstreamertypes;

/* phobos */
import std.path, std.conv, std.range, std.stdio, std.file, std.string;
import std.container, std.array;

/* DumbPlayer modules */
import player;
import playlist;
import settings;

final class DumbPlayer
{
	public this(in string[] args, in Settings settings) {
		import gtk.StyleContext;
		import gdk.RGBA;
		import std.process, std.stdio; 
		
		immutable GTK_STYLE_REGION_ROW = "row";
		immutable GTK_STYLE_CLASS_VIEW = "view";
	
		m_player = new Player();
		m_player.statusHandler = &onPlayerStateChanged;
		m_player.errorHandler = &onPlayerError;
		
		m_prevSong = null;
		
		string envConfigHome;
		try
			envConfigHome = environment["XDG_CONFIG_HOME"];
		catch
			envConfigHome = environment["HOME"] ~ "/.config/";
		m_playlistPath = envConfigHome ~ settings.PLAYLIST_FILE;
		
		try {
			if(!initUI(settings.UI_FILE, (settings.UI_XML=="") ? null : settings.UI_XML))
				throw new Exception("initUI() failed");
				
			initDnD();
		}
		catch {
			throw new Exception("Initialization failed");
		}
		
		RGBA hl = new RGBA();
		RGBA bg = new RGBA();
		
		StyleContext context = m_ui_playlist.getStyleContext();
		
		context.save();
			context.addClass(GTK_STYLE_CLASS_VIEW);
			context.getBackgroundColor(GtkStateFlags.NORMAL, bg);
			context.addRegion(GTK_STYLE_REGION_ROW, GtkRegionFlags.ODD);
			context.getBackgroundColor(GtkStateFlags.SELECTED, hl);
		context.restore();
		
		m_hlColor = GdkRGBA(
			(hl.red()   + bg.red())   /2 ,
			(hl.green() + bg.green()) /2 ,
			(hl.blue()  + bg.blue())  /2 ,
			 hl.alpha
		);
			
		m_bgColor = GdkRGBA(
			bg.red(),
			bg.green(),
			bg.blue(),
			bg.alpha()
		);
		
		m_player.playlist.defaultHighlight = m_bgColor;
		
		Playlist.PathErrorHandler errorHandler = 
		delegate void(in string path, in ErrorG error) {
			stderr.writeln (
				"ERROR: Error while reading path: \"", path, "\"", 
				"\nMessage: ", to!(string)((cast(ErrorG)(error)).getErrorGStruct().message)
			);
		};
		
		foreach(string s; drop(args,1))
			m_player.add(s, errorHandler);
		
		m_ui_mainWindow.show();
	}
	
	private bool initUI(in string uiFile, in string uiXML = null) {
		import gtk.IconTheme;
		
		m_ui_builder = new Builder();
		
		if(uiXML is null && !m_ui_builder.addFromFile(uiFile)) {
			stderr.writeln("Cannot open UI file: ", uiFile);
			return false;
		}
		else if(uiXML !is null && !m_ui_builder.addFromString(uiXML, uiXML.length)) {
			stderr.writeln("Cannot initialize UI");
			return false;
		}
		
		m_ui_mainWindow = cast(Window) m_ui_builder.getObject("mainWindow");
		m_ui_mainWindow.addOnDestroy(&onMainWinDestroy);
		
		if(IconTheme.getDefault().hasIcon("dumbplayer"))
			m_ui_mainWindow.setIconName("dumbplayer");
		else
			m_ui_mainWindow.setIconName("applications-multimedia");
		
		with(m_ui_builder)
		{
			//Buttons
			(m_ui_toggleButton = cast(ToolButton) getObject("play")).addOnClicked(&onButtonPlayClick);
			(m_ui_stopButton = cast(ToolButton) getObject("stop")).addOnClicked(&onButtonStopClick);
			(cast(MenuToolButton) getObject("next-menu")).addOnClicked(&onButtonNextClick);
			(cast(ToolButton) getObject("prev")).addOnClicked(&onButtonPrevClick);
			
			//Playback mode menu
			(cast(RadioMenuItem) getObject("modeLinear")).addOnActivate(&onModeLinear);
			(cast(RadioMenuItem) getObject("modeRandom")).addOnActivate(&onModeRandom);
			(cast(RadioMenuItem) getObject("modeSingle")).addOnActivate(&onModeSingle);

			//Position slider and label
			m_ui_timeLabel = cast(Label) getObject("timeLabel");

			m_ui_timeRange = cast(Scale) getObject("position");
			m_ui_timeRange.setRange(0.0, 1.0);
			m_ui_timeRange.addOnButtonRelease(&onRangeMouseUp);
			m_ui_timeRange.addOnButtonPress(&onRangeMouseDown);
			m_ui_timeRange.addOnValueChanged(&onRangeChangeValue);

			//Playlist listbox
			m_ui_playlist = cast(TreeView) getObject("playlist");
			m_ui_playlist.addOnRowActivated(&onRowActivated);
			m_ui_playlist.setModel(cast(ListStore) m_player.playlist);
			m_ui_playlist.addOnButtonPress(&onPlaylistClick);
			m_ui_playlist.addOnPopupMenu(delegate bool(Widget w){
				onPlaylistClick(null, w); 
				return true;
			});
			
			//Right-click menu
			m_ui_playlistMenu = cast(Menu) getObject("playlistMenu");
			m_ui_playlistMenu.attachToWidget(m_ui_playlist, null);
			
			//Errors display widget
			m_ui_errorMessage = cast(InfoBar) getObject("errorMsg");
			m_ui_errorMessage.addButton(StockID.CLOSE, GtkResponseType.CLOSE);
			m_ui_errorMessage.addOnResponse(&onInfoBarPress);
			m_ui_errorMsgLabel = cast(Label) getObject("errorText");
		}
		
		import glib.ListG;
		ListG menuItems = m_ui_playlistMenu.getChildren();
		
		m_ui_playlistSave = new MenuItem(cast(GtkMenuItem*) menuItems.data);
		m_ui_playlistRemove = new MenuItem(cast(GtkMenuItem*) menuItems.next().data);
		m_ui_playlistClear = new MenuItem(cast(GtkMenuItem*) menuItems.next().next().data);
		
		m_ui_playlistSave.addOnActivate(&onPlaylistSave);
		m_ui_playlistRemove.addOnActivate(&onPlaylistRemove);
		m_ui_playlistClear.addOnActivate(&onPlaylistClear);
		
		m_ui_timeRange.setSensitive(0);
		m_ui_stopButton.setSensitive(0);
		
		m_positionTimer = new Timeout(&onPositionTimer, 1);
		
		
		return true;
	}
	
	private void initDnD() {
		GtkTargetEntry[2] dndTargets = [
			GtkTargetEntry( Str.toStringz("text/uri-list"), 0, DND_URI_ID),
			GtkTargetEntry( Str.toStringz("application/x-playlist-row"), 0, DND_SONG_ID)
		];
		
		m_ui_playlist.enableModelDragDest(dndTargets, DragAction.ACTION_COPY | DragAction.ACTION_MOVE);
		m_ui_playlist.enableModelDragSource (
			GdkModifierType.BUTTON1_MASK,
			[dndTargets[1]],
			DragAction.ACTION_MOVE
		);
		
		m_ui_playlist.addOnDragDataReceived(&onDnDData);
	}

	private void setSong(in TreeRowReference row) {
		if(row is null) {
			m_ui_mainWindow.setTitle("DumbPlayer");
			return;
		}
		
		if(m_prevSong)
			m_player.playlist.highlightValue(m_prevSong, m_bgColor);
			
		m_player.playlist.highlightValue(row, m_hlColor);
		
		m_prevSong = (cast(TreeRowReference)(row)).copy();
		
		m_ui_timeRange.setRange(0.0, m_player.duration/10.0);
		m_ui_timeRange.setIncrements(0.1, 1.0);
		m_ui_timeRange.setSensitive(1);
		
		m_ui_mainWindow.setTitle (
			m_player.playlist.value(row, Playlist.Column.ARTIST) ~ 
			" - " ~
			m_player.playlist.value(row, Playlist.Column.TITLE)
		);
	}
	
	private bool onPositionTimer() {
		if(m_player.status == m_player.Status.PLAYING && !m_isSeeking) {
			long len = m_player.duration;
			long pos = m_player.position;
			
			setPositon(pos, len);
		}
		return true;
	}
	
	private void setPositon(long pos, long len) {	
		m_ui_timeLabel.setText(
			format("%s%d:%s%d / %s%d:%s%d",
			(pos/60 < 10) ? "0" : "", pos/60,
			(pos%60 < 10) ? "0" : "", pos%60,
			(len/60 < 10) ? "0" : "", len/60,
			(len%60 < 10) ? "0" : "", len%60)
		);
			
		m_ui_timeRange.setValue(pos/10.0);	
	}
	
	private void toggleErrorMessage() {
		if(!m_errorMessages.empty && m_ui_errorMessage.getVisible() == 0) {
			m_ui_errorMsgLabel.setText(m_errorMessages.back());
			m_errorMessages.removeBack();
			m_ui_errorMessage.show();
		}
	}
	
	private void onMainWinDestroy(Widget) {
		Main.quit();
	}
	
	bool onRangeMouseDown(Event ev, Widget w) {	
		m_isSeeking = true;
		return false;
	}
	
	bool onRangeMouseUp(Event ev, Widget w) {
		m_player.position = cast(uint) (m_ui_timeRange.getValue()*10.0);
		m_isSeeking = false;
		return false;
	}
	
	void onRangeChangeValue(Range rng) {
		setPositon(to!(long)(rng.getValue()*10.0), m_player.duration);
	}
	
	private void onRowActivated(TreePath path, TreeViewColumn, TreeView v) {
		m_player.play(path);
	}
		
	private void onButtonPlayClick(ToolButton) {
		final switch(m_player.status) {
			case m_player.Status.PLAYING: m_player.pause(); break;
			case m_player.Status.PAUSED: m_player.resume(); break;
			case m_player.Status.STOPPED: m_player.play(); break;
		}
	}
	
	private void onButtonStopClick(ToolButton) {
		m_player.stop();
		
		if(m_prevSong) {
			m_player.playlist.highlightValue(m_prevSong, m_bgColor);
			m_prevSong = null;
		}
		
		m_ui_stopButton.setSensitive(0);
		m_ui_timeRange.setSensitive(0);
	}
	
	private void onButtonNextClick(ToolButton) {
		m_player.next();
	}
	
	private void onButtonPrevClick(ToolButton) {
		m_player.prev();
	}
	
	private void onModeLinear(MenuItem) {
		m_player.mode = Player.Mode.LINEAR;
	}
	
	private void onModeRandom(MenuItem) {
		m_player.mode = Player.Mode.RANDOM;
	}
	
	private void onModeSingle(MenuItem) {
		m_player.mode = Player.Mode.SINGLE;
	}
	
	private void onInfoBarPress(int, InfoBar) {
		m_ui_errorMessage.hide();
		toggleErrorMessage();
	}
	
	private bool onPlaylistClick(Event ev, Widget) {
		if(ev is null) {
			m_ui_playlistMenu.popup(0, Main.getCurrentEventTime());
			return false;
		}
		
		if(ev.triggersContextMenu() && ev.type == GdkEventType.BUTTON_PRESS) {
			if(m_player.playlist.length < 1) {
				m_ui_playlistRemove.setSensitive(0);
				m_ui_playlistClear.setSensitive(0);
			}
			else if(!m_ui_playlist.getSelectedIter()) {
				m_ui_playlistRemove.setSensitive(0);
				m_ui_playlistClear.setSensitive(1);
			}
			else if(!m_ui_playlistRemove.getSensitive() || !m_ui_playlistClear.getSensitive()) {
				m_ui_playlistRemove.setSensitive(1);
				m_ui_playlistClear.setSensitive(1);
			}
			
			uint mouseButton;
			ev.getButton(mouseButton);
			m_ui_playlistMenu.popup(mouseButton, ev.getTime());
		}
		
		return false;
	}
	
	private void onPlaylistRemove(MenuItem) {
		m_player.playlist.remove(m_ui_playlist.getSelectedIter());
		if(m_prevSong && !m_prevSong.valid())
			m_prevSong = null;
	}
	
	private void onPlaylistClear(MenuItem) {
		m_player.playlist.clear();
		m_prevSong = null;
	}
	
	private void onPlaylistSave(MenuItem) {
		m_player.playlist.loadFromFile(m_playlistPath);
	}
	
	private void onDnDData(DragContext dc, int x, int y, SelectionData data, 
							uint info, uint time, Widget widget)
	{
		import std.uri;
		
		if(info == DND_URI_ID) {
			string[] URIs = data.dataGetUris();
			
			foreach(string s; URIs)
				m_player.add(decode(s), cast(Playlist.PathErrorHandler)(&onPlayerPathError));
		}
		else if(info == DND_SONG_ID) {
			TreeIter src = m_ui_playlist.getSelectedIter();
			
			TreePath dstPath = new TreePath();
			GtkTreeViewDropPosition dstDropType;
			m_ui_playlist.getDestRowAtPos(x, y, dstPath, dstDropType);
			
			TreeIter dst = new TreeIter();
			m_ui_playlist.getModel().getIter(dst, dstPath);
			
			if(dstDropType == GtkTreeViewDropPosition.BEFORE || 
				dstDropType == GtkTreeViewDropPosition.INTO_OR_BEFORE)  {
				(cast(ListStore)(m_player.playlist)).moveBefore(src,dst);
			}
			else {
				(cast(ListStore)(m_player.playlist)).moveAfter(src,dst);
			}
		}
		else assert(0);
	}
	
	private void onPlayerStateChanged(in Player.Status status) {
		setSong(m_player.currentSong);
		
		if(status == m_player.Status.PLAYING) {
			m_ui_toggleButton.setStockId("gtk-media-pause");

			m_ui_stopButton.setSensitive(1);
			m_ui_timeRange.setSensitive(1);
			onPositionTimer();
		}
		else {
			m_ui_toggleButton.setStockId("gtk-media-play");
			
			if(status == m_player.Status.STOPPED) {
				m_ui_stopButton.setSensitive(0);
				m_ui_timeRange.setSensitive(0);
				m_ui_timeRange.setValue(0.0);
				m_ui_timeLabel.setText("00:00 / 00:00");
			}
		}
	}
	
	private void onPlayerError(in ErrorG error) {
		m_errorMessages.insertBack (
			"An error occured:\n" ~
			to!string((cast(ErrorG) error).getErrorGStruct().message)
		);
		toggleErrorMessage();
	}
	
	private void onPlayerPathError(in string path, in ErrorG error) {
		m_errorMessages.insertBack (
			"Error while reading path: " ~ path ~ "\n" ~ 
			to!string((cast(ErrorG) error).getErrorGStruct().message)
		);
		toggleErrorMessage();
	}
	
	private const DND_URI_ID = 80;
	private const DND_SONG_ID = 42;
	
	private Builder m_ui_builder;	
	private Window m_ui_mainWindow;
	private ToolButton m_ui_toggleButton;
	private ToolButton m_ui_stopButton;
	private TreeView m_ui_playlist;
	private Label m_ui_timeLabel;
	private Scale m_ui_timeRange;
	private Menu m_ui_playlistMenu;
	private MenuItem m_ui_playlistSave;
	private MenuItem m_ui_playlistRemove;
	private MenuItem m_ui_playlistClear;
	private InfoBar m_ui_errorMessage;
	private Label m_ui_errorMsgLabel;
	
	private Timeout m_positionTimer;
	
	private DList!(string) m_errorMessages;
	
	private GdkRGBA m_hlColor;
	private GdkRGBA m_bgColor;
	
	private TreeRowReference m_prevSong;
	private bool m_isSeeking = false;
	private string m_playlistPath;
	
	private Player m_player;
}