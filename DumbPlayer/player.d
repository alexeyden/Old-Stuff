module player;

import gtk.TreeIter;
import gtk.ListStore;
import gtk.TreePath;

import gtkc.gtktypes;

import gobject.Value;

import gstreamer.gstreamer;
import gstreamer.Element;
import gstreamer.ElementFactory;
import gstreamer.Message;
import gstreamer.Format;
import gstreamerc.gstreamertypes;

import std.path;
import std.file;
import std.string;
import std.uri;
import std.stdio;
import std.conv;

import std.c.stdlib;

class Player
{	
	protected:
		enum PlayStatus
		{	
			PLAYING,
			PAUSED,
			STOPPED
		}
		enum PlayMode
		{
			LINEAR,
			SINGLE,
			RANDOM
		}
		
		Element source;
		ListStore playlist;
		TreeIter now_playing;
		PlayStatus status;
		PlayMode mode;
		
		void onError() {}
		void onDurationChange() {};
		void onEOS() { next(); }
		
		bool onGstMessage(Message msg)
		{
			switch(msg.type)
			{
				case GstMessageType.EOS:
					onEOS();
					break;
				case GstMessageType.ERROR:
					this.stop();
					onError();
					break;
				case GstMessageType.NEW_CLOCK:
					onDurationChange();
					break;
				default:
					break;
			}
			
			return true;
		} 
		
		
	public:	
		this()
		{
			status = PlayStatus.STOPPED;
			mode = PlayMode.LINEAR;
			
			GType[] g = [ GType.INT, GType.STRING, GType.STRING];
			playlist = new ListStore(g);
			
			source = ElementFactory.make("playbin2","player");
			Element fake_sink = ElementFactory.make("fakesink","video_fake_sink");
			
			source.getBus().addWatch(&onGstMessage);
			
			Value v = new Value();
			v.init(GType.OBJECT);	
			v.setObject( fake_sink.getElementStruct() );		
			
			source.setProperty("video-sink",v);
		}
		~this()
		{
			stop();
		}
		
		/* playing control */
		void play(TreeIter it)
		{
			if(now_playing)
				playlist.setValue(now_playing, 0, false);
			
			stop();
			
			string to_play = "file://" ~ it.getValueString(2);
			source.setProperty("uri",to_play);
			
			this.status = PlayStatus.PLAYING;
			source.setState(GstState.PLAYING);
			
			now_playing = it;
			
			playlist.setValue(it, 0, true);
		}
		
		void toggle()
		{
			if(status == PlayStatus.PAUSED)
			{
				this.status = PlayStatus.PLAYING;
				source.setState(GstState.PLAYING);
			}
			else if(status == PlayStatus.PLAYING)
				this.pause();
		}
		
		void pause()
		{
			this.status = PlayStatus.PAUSED;
			source.setState(GstState.PAUSED);
		}
		
		void stop()
		{
			this.status = PlayStatus.STOPPED;
			source.setState(GstState.NULL);
			if(now_playing)
				playlist.setValue(now_playing, 0, false);
			now_playing = null;
		}
		
		void next()
		{			
			if(now_playing)
			{
				TreeIter it = new TreeIter(playlist,now_playing.getTreePath());
			
				switch(mode)
				{
					case PlayMode.LINEAR:
						if(!playlist.iterNext(it))
							playlist.getIterFirst(it);
					break;
					case PlayMode.SINGLE:
						stop();
					break;
					case PlayMode.RANDOM:
						int count = playlist.iterNChildren(null);
						if(count > 0)
						{
							int next_i = rand()%count;
							playlist.iterNthChild(it,null,next_i);
						}
					break;
				}
				
				play(it);
			}
		}
		
		void prev()
		{
			if(now_playing)
			{
				TreeIter it = new TreeIter(playlist,now_playing.getTreePath());
				
				switch(mode)
				{
					case PlayMode.LINEAR:
						TreePath p = it.getTreePath();
						if(!p.prev())
						{
							p.free();
							delete p;
							int last = playlist.iterNChildren(null);
							playlist.iterNthChild(it,null,last-1);
						}
						else
							playlist.getIter(it,p);
					break;
					case PlayMode.SINGLE:
						stop();
					break;
					case PlayMode.RANDOM:
						int count = playlist.iterNChildren(null);
						if(count > 0)
						{
							int next_i = rand()%count;
							playlist.iterNthChild(it,null,next_i);
						}
					break;
				}
				
				play(it);
			}
		}
		
		/* playlist */
		void add(string path)
		{	
			if(isdir(path))
			{
				foreach(string s; listdir(path))
				{
					this.add(path~"/"~s);
				}
			}
			else
			{
				TreeIter it = new TreeIter();
				this.playlist.append(it);
	
				playlist.setValue(it, 0, false);
				playlist.setValue(it, 1, basename(path));
				playlist.setValue(it, 2, path);
			}
		}
		
		bool remove(TreeIter it)
		{
			if(this.playlist.remove(it))
				return true;
			else
				return false;
		}
		
		/* play mode */
		void setMode(PlayMode m) { mode = m; }
		PlayMode getMode() { return mode; }
		
		/* time */
		long getDuration() {
				return source.queryDuration() / GST_SECOND;
		}
		long getPosition() {
				return source.queryPosition() / GST_SECOND;
		}
		void setPosition(uint seconds) {
			source.seek(1.0,GstFormat.TIME,GstSeekFlags.NONE,
						GstSeekType.SET,seconds*GST_SECOND,GstSeekType.END,0);
		}
}
