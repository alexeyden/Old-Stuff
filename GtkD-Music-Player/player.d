module player;

/* GStreamer */
import gstreamer.gstreamer;
import gstreamer.Element;
import gstreamer.Message;

/* Gtk */
import gtk.TreePath;
import gtk.TreeRowReference;
import gtk.TreeModelIF;
import gtk.ListStore;
import gtk.TreeIter;
import glib.ErrorG;

/* Phobos */
import std.container;
import std.path;
import std.conv;
import std.random;
import std.string;

import playlist;

class Player {
	public enum Status {
		PLAYING,
		PAUSED,
		STOPPED
	}

	public enum Mode {
		LINEAR,
		SINGLE,
		RANDOM
	}

	public this() {
		import gstreamer.ElementFactory;
		import gobject.Value;
		
		m_playlist = new Playlist();
		
		(cast(ListStore)(m_playlist)).addOnRowDeleted(&onPlaylistDeleteRow);

		m_status = Status.STOPPED;
		m_mode = Mode.LINEAR;
		m_isSeeking = false;
		
		m_currentSong = null;

		m_errorCallback = null;
		m_statusCangeCallback = null;

		m_playbin = ElementFactory.make("playbin2","player");
		Element fakeVideoSink = ElementFactory.make("fakesink");

		m_playbin.getBus().addWatch(&onGstMessage);

		Value value = new Value();
		value.init(GType.OBJECT);
		value.setObject(fakeVideoSink.getElementStruct());

		m_playbin.setProperty("video-sink",value);
	}

	public ~this() {
		stop();
	}
	
	public void add(in string path, in Playlist.PathErrorHandler handler = null) {
		m_playlist.add(path, handler);
	}
	
	/** Stop and then set state to playing */
	public void play() {
		if(m_playlist.length == 0)
			return;
		if(m_currentSong is null)
			return;

		stop();
			
		gstPlay(m_currentSong);
	}
	
	/** Play Nth song in playlist */
	public void play(in uint index) {
		if(index < m_playlist.length) {
			if (
				m_mode == Mode.RANDOM &&
				m_currentSong !is null && (
					m_previousSongs.empty || 
					m_playlist[index].getPath().compare(m_previousSongs.back().getPath()) != 0
				)
			) m_previousSongs.insertBack(m_currentSong);
			
			m_currentSong = m_playlist[index];
			play();
		}
		else throw new Exception("Out of range");
	}
	
	/** Play a song @path */ 
	public void play(in TreePath path) {
		uint index = (cast(TreePath)(path)).getIndices()[0];
		play(index);
	}
	
	/** Play next song. In single and linear modes this method will play
	 * a song that is next to the current one in playlist
	 * In random mode a random song will be played.*/
	public void next() {
		if(m_playlist.length == 0)
			return;
		
		final switch(m_mode) {
			case Mode.SINGLE:
			case Mode.LINEAR:
				if(m_currentSong is null)
					return;
			
				auto nextRow = m_playlist.rowNext(m_currentSong);
				if(nextRow is null)
					m_currentSong = m_playlist.getRow(0);
				else
					m_currentSong = nextRow;
				play();
			break;
			case Mode.RANDOM:
				play(uniform(0, m_playlist.length-1));
			break;
		}
	}
	
	/** Play previous song. */
	public void prev() {
		if(m_playlist.length == 0)
			return;

		stop();

		if(m_mode == Mode.RANDOM) {
			if(m_previousSongs.empty) {
				m_currentSong = m_playlist.getRow(uniform(0, m_playlist.length-1));
				play();
			}
			else {
				m_currentSong = m_previousSongs.back();
				m_previousSongs.removeBack();
				
				play();
			}
		}
		else {
			if(m_currentSong is null)
				return;
				
			auto prevRow = m_playlist.rowPrev(m_currentSong);
			if(prevRow is null)
				m_currentSong = m_playlist.getRow(m_playlist.length-1);
			else
				m_currentSong = prevRow;
				
			play();
		}
	}

	public void pause()	{
		if(status() == Status.PLAYING) {
			m_status = Status.PAUSED;
			m_playbin.setState(GstState.PAUSED);
		}
	}
	
	public void resume() {
		if(status() == Status.PAUSED) {
			m_status = Status.PLAYING;
			m_playbin.setState(GstState.PLAYING);
		}
	}
	
	public void stop() {
		m_status = Status.STOPPED;
		m_playbin.setState(GstState.NULL);
		
		if(m_statusCangeCallback)
			m_statusCangeCallback(Status.STOPPED);
	}

	@property
	public TreeRowReference currentSong() {
		if(m_status == Status.STOPPED)
			return null;
		return m_currentSong;
	}
	
	@property
	public void errorHandler(in void delegate(in ErrorG) handler) {
		m_errorCallback = handler;
	}
	
	@property
	public void statusHandler(in void delegate(in Status) handler) {
		m_statusCangeCallback = handler;
	}
	
	@property
	public Playlist playlist() {
		return m_playlist;
	}

	@property
	public void mode(in Mode newMode) {
		if(newMode == Mode.RANDOM && m_mode != Mode.RANDOM)
			m_previousSongs.clear();
		m_mode = newMode;
	}
	
	@property
	public Mode mode() const {
		return m_mode;
	}

	@property
	public Status status() const {
		return m_status;
	}

	@property
	public ulong position() {
		return m_playbin.queryPosition() / GST_SECOND;
	}

	@property
	public void position(uint seconds) {
		m_isSeeking = true;
		/* This causes some seeking lags for some reason:
		m_playbin.seek(
			1.0, GstFormat.TIME, GstSeekFlags.FLUSH,
			GstSeekType.SET, seconds * GST_SECOND,
			GstSeekType.END, GST_CLOCK_TIME_NONE
		);
		*/
		m_playbin.seekSimple(GstFormat.TIME, GstSeekFlags.FLUSH, seconds * GST_SECOND);
	}
	
	@property
	public ulong duration() {
		return m_playbin.queryDuration() / GST_SECOND;
	}
	
	private void gstPlay(in TreeRowReference row) {
		string songURI = m_playlist.value(row, Playlist.Column.PATH);

		m_playbin.setProperty("uri", songURI);
		
		m_status = Status.PLAYING;
		m_playbin.setState(GstState.PLAYING);
	}
	
	private void onPlaylistDeleteRow(TreePath path, TreeModelIF model) {
		import std.range;
		
		size_t i = 0;
		foreach(TreeRowReference row; m_previousSongs) {
			if(!row.valid()) {
				m_previousSongs.linearRemove(take(drop(m_previousSongs[],i), 1));
				continue;
			}
			i++;
		}
		if(m_currentSong && !m_currentSong.valid())
			m_currentSong = null;
	}
	
	private bool onGstMessage(Message msg) {
		switch(msg.type) {
			case GstMessageType.ERROR: {
				if(m_errorCallback) {
					ErrorG err;
					string s;
					msg.parseError(err,s);
					m_errorCallback(err);
				}
			}
			break;
			case GstMessageType.EOS:
				if(m_mode == Mode.SINGLE)
					play();
				else
					next();
				
				break;
			case GstMessageType.STATE_CHANGED:
				if (
					msg.getMessageStruct().src != 
					m_playbin.getObjectGstStruct()
				) break;
				
				GstState oldState, newState;
				msg.parseStateChanged(&oldState, &newState, null);
				
				Status st = Status.STOPPED;
				switch(newState) {
					case GstState.PLAYING:
						if(m_isSeeking) {
							m_isSeeking = false;
							goto done;
						}
						st = Status.PLAYING;
					break;
					case GstState.PAUSED:
						if(m_isSeeking)
							goto done;
						if(oldState == GstState.READY)
							goto done;
						
						st = Status.PAUSED;
					break;
					default: goto done;
				}
				
				if(m_statusCangeCallback)
					m_statusCangeCallback(st);
					
				done:
				break;
			default:
				break;
		}

		return true;
	}
	
	protected Playlist m_playlist;
	private Status m_status;
	private Mode m_mode;
	private bool m_isSeeking;

	private DList!(TreeRowReference) m_previousSongs;
	private TreeRowReference m_currentSong;

	protected Element m_playbin;

	private void delegate(in ErrorG) m_errorCallback;
	private void delegate(in Status) m_statusCangeCallback;
}