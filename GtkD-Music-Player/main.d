import dumbplayer;
import settings;
import config;

import std.stdio;

import gstreamer.gstreamer;
import gtk.Main;

int main(string[] args)
{
	if(args.length > 1 && (args[1] == "-h" || args[1] == "--help")) {
		writeln(
			"Usage: \n\t", args[0], " [OPTIONS] [PATH]...\n\n",
			"Avaliable options: \n",
			"\t-h, --help\t\tPrint this message",
		);
		return 0;
	}

	Main.init(args);
	GStreamer.init(args);
	
	try {
		new DumbPlayer(args, CONFIG_SETTINGS);
	}
	catch(Exception ex){
		stderr.writeln("ERROR: ", ex.msg);
		return 1;
	}
		
	
	Main.run();
	
	return 0;
}