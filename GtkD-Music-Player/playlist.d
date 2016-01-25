module playlist;

/* Gstreamer */
import gstreamer.gstreamer;
import gstreamer.Element;
import gstreamer.Message;
import gstreamer.Pipeline;

import gstreamerc.gstreamertypes;

/* Gtk */
import gtk.ListStore;
import gtk.TreeModel;
import gtk.TreePath;
import gtk.TreeIter;
import gtk.TreeRowReference;
import gtk.TreeView;
import gtk.TreeViewColumn;
import gtk.TreeModelIF;

/* Phobos */
import std.container;
import std.conv;

/* GLib and GObject */
import gobject.Value;
import glib.Str;
import glib.ErrorG;
import gtkc.gobject;

/* It seems like GtkD doesnt wrap this function
 * so i'm taking it directly from libgdk-3 */
static extern(C) GType gdk_rgba_get_type();

class Playlist {
	public alias void delegate(in string path, in ErrorG error) PathErrorHandler;
	
	public enum Column {
		TITLE     = 1,
		ALBUM     = 2,
		ARTIST    = 3,
		DURATION  = 4,
		PATH      = 5,
		HIGHLIGHT = 0
	}
	
	public this() {		
		import gstreamer.ElementFactory, gstreamer.Pad;
		
		GType[] columns = [
			gdk_rgba_get_type(), //color of a row
			GType.STRING, //title
			GType.STRING, //album
			GType.STRING, //artist
			GType.STRING, //duration
			GType.STRING  //path
		];
		m_list = new ListStore(columns);
		
		/* create pipeline uridecoder -> fakesink */
		m_gstPipeline = new Pipeline("pipeline");
		m_gstPipeline.setState(GstState.NULL);

		m_gstDecoder = ElementFactory.make("uridecodebin",null);
		m_gstPipeline.add(m_gstDecoder);

		auto gstSink = ElementFactory.make("fakesink",null);
		m_gstPipeline.add(gstSink);
		
		/* connect all decoder source pads to fakesink input pad */
		m_gstDecoder.addOnPadAdded(delegate void(Pad pad, Element decoder) {
			pad.link(gstSink.getStaticPad("sink"));
		});
	}
	
	public TreeIter add(in string path, in PathErrorHandler onError = null)  {
		import std.regex, std.path, std.file, std.string, std.range, std.algorithm;
		import gstreamer.TagList; 
		import gio.ContentType;
		
		static auto urlRegex = regex(r"^(.{1,}:/).");
		
		string urlPath = path;
		if(match(path, urlRegex).empty)
			urlPath = "file://" ~ urlPath;
			
		immutable size_t fileLen = "file://".length;
		if( equal(take(urlPath,fileLen), "file://") &&
			exists(urlPath[fileLen..$]) &&
			isDir(urlPath[fileLen..$])
		){
			foreach(string s; dirEntries(urlPath[fileLen..$],SpanMode.depth))
				add("file://" ~ s, onError);	
			return null;
		}
		
		if(equal(take(urlPath,fileLen), "file://")) {
			int certainty;
			string mimeType = ContentType.guess(drop(urlPath, fileLen), null, certainty);
			if(!equal(take(mimeType,"audio/".length),"audio/"))
				return null;
		}
		
		m_gstDecoder.setProperty("uri",urlPath);
		m_gstPipeline.setState(GstState.PAUSED);
		
		TagList tags = new TagList();

		while(true) {
			Message msg = m_gstPipeline.getBus().timedPop(GST_CLOCK_TIME_NONE); 
				
			/* if file doesnt exist or wrong format or smth */
			if(msg.type == GstMessageType.ERROR) {
				m_gstPipeline.setState(GstState.NULL);
				
				ErrorG err;
				string s;
				msg.parseError(err,s);
				
				if(onError)
					onError(urlPath, err);
					
				return null;
			}

			if(msg.type() == GstMessageType.TAG) {
				/* TAG message can be sent multiple times,
				so we handle them all by merging all tags into one taglist */
				tags = tags.merge(msg.parseTag(), GstTagMergeMode.APPEND);
			}
			
			if(msg.type() == GstMessageType.ASYNC_DONE)
				break;
		}
		
		string[Column.max] song;
		tags.getString("title", song[Column.TITLE  -1]);
		tags.getString("album", song[Column.ALBUM  -1]);
		tags.getString("artist",song[Column.ARTIST -1]);
		
		//if there is no title tag
		if(!song[Column.TITLE -1]) {
			if(equal(take(urlPath,fileLen), "file://"))
				song[Column.TITLE -1] = baseName(urlPath[fileLen..$]);
			else
				song[Column.TITLE -1] = urlPath;
		}

		ulong duration = m_gstDecoder.queryDuration() / GST_SECOND;

		ulong min = duration / 60;
		ulong sec = duration % 60;
		string strDuration = to!(string)(min) ~ ":" ~
			((sec < 10) ? "0" : "") ~ to!string(sec);

		song[Column.DURATION -1] = strDuration;
		song[Column.PATH -1] = urlPath;
			
		m_gstPipeline.setState(GstState.NULL);
		
		return listStoreAppend(m_background, song);
	}
	
	/** Clear playlist */
	public void clear() {
		m_list.clear();
	}
	
	/** Remove row @iter */
	public bool remove(TreeIter iter) {
		return (m_list.remove(iter)) ? true : false;
	}
	
	/** Get row next to @row */
	public TreeRowReference rowNext(TreeRowReference row) {
		auto path = row.getPath();
		path.next();
		if(path.getIndices()[0] >= length())
			return null;
		return new TreeRowReference(m_list, path);
	}
	
	/** Get previous row for @row */
	public TreeRowReference rowPrev(TreeRowReference row) {
		auto path = row.getPath();
		if(path.prev())
			return new TreeRowReference(m_list, path);
			
		return null;
	}
	
	/** Get Iterator by index @index */
	public TreeIter getIter(uint index) {
		TreeIter it = new TreeIter();
		return (m_list.iterNthChild(it, null, index) ? it : null);
	}
	
	/** Get RowReference by index @index */
	public TreeRowReference getRow(uint index) {
		TreeIter it = getIter(index);
		if(it is null)
			return null;
		TreeRowReference rowRef = new TreeRowReference(m_list, m_list.getPath(it));
		return rowRef;
	}
	
	/** Check if playlist contains a row @it */
	public bool has(TreeIter it) {
		return (m_list.iterIsValid(it)) ? true : false;
	}
	
	public TreeRowReference opIndex(uint index) {
		return getRow(index);
	}
	
	public ListStore opCast(ListStore)() {
		return m_list;
	}
	
	/** Get value of column @col in row @it */
	public string value(in TreeIter it, Column col) {
		return m_list.getValueString(cast(TreeIter) it, cast(int) col);
	}
	
	/** Get value of column @col in row @rowRef */
	public string value(in TreeRowReference rowRef, Column col) {
		TreeIter it = new TreeIter();
		m_list.getIter(it, (cast(TreeRowReference)rowRef).getPath());
		
		return value(it, col);
	}
	
	/** Set color of @row to @value */
	public void highlightValue(in TreeRowReference row, in GdkRGBA value) {
		TreeIter it = new TreeIter();
		m_list.getIter(it, (cast(TreeRowReference)(row)).getPath());
		
		Value v = new Value();
		v.init(gdk_rgba_get_type());
		v.setBoxed(cast(GdkRGBA*) &value);
		
		m_list.setValue(it, 0, v);
	}
	
	/** Save playlist to file @path in m3u format */
	public bool saveToFile(in string path) {
		import std.stdio;
		import std.range;
		import std.algorithm;
		
		File output;
		try
			output = File(path, "w");
		catch
			return false;
			
		output.writeln("# DumbPlayer playlist");
		for(size_t i = 0; i < this.length; i++) {
			string songPath = value(getIter(i), Column.PATH);
			if(equal(take(songPath,"file://".length), "file://"))
				output.writeln(drop(songPath,"file://".length));
			else
				output.writeln(songPath);
		}
		output.close();
		
		return true;
	}
	
	/** Restore playlist from file @path */
	public bool loadFromFile(in string path) {
		import std.stdio;
		
		File input;
		try
			input = File(path, "r");
		catch
			return false;
			
		string line = "";
		do {
			if(line != "" && line[0] != '#' && line[0] != ' ') {
				add(line[0..$-1]);
			}
		} while(input.readln(line));
		
		input.close();
		
		return true;
	}
	
	/** Set default row color */
	@property
	public void defaultHighlight(in GdkRGBA bg) {
		m_background = bg;
	}
	
	@property
	public size_t length() {
		return m_list.iterNChildren(null);
	}
	
	private TreeIter listStoreAppend(ref GdkRGBA color,ref string[Column.max] values) {
		TreeIter it = new TreeIter();
		m_list.append(it);
		
		Value v = new Value();
		v.init(gdk_rgba_get_type());
		v.setBoxed(&color);
		m_list.setValue(it, 0, v);
		
		foreach(size_t i, string val; values)
			m_list.setValue(it, i+1, val);
		
		return it;
	}
	
	protected ListStore m_list;
	protected Pipeline m_gstPipeline;
	protected Element m_gstDecoder;
	protected GdkRGBA m_background = { 1.0, 1.0, 1.0, 1.0 };
}