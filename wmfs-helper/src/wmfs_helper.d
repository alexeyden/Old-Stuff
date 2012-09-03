import std.stdio;
import std.datetime;
import std.conv;
import std.c.linux.linux;
import std.string;
import std.array;

import funcs;

extern(C)
{
	uint wmfs_set_status(immutable(char)* s);
	uint wmfs_is_running();
}

bool debugging = false;
uint upd_interval = 1;

void main_loop(uint upd_time,string status)
{
	while(true)
	{
		status = status.replace("%D",get_date());
		status = status.replace("%T",get_time());
		status = status.replace("%V",get_volume());
		status = status.replace("%B",get_battery());

		if(debugging)
			writeln(status);
		else
			wmfs_set_status(toStringz(status));

		sleep(upd_time);
	}
}

int main(string args[])
{
	if(!wmfs_is_running())
	{
		writeln("WMFS is not running!");
		return 1;
	}

	string status_string = "[%T]";
	


	foreach(string arg;args)
	{	
		//Enable debugging
		if(arg == "-d")
			debugging = true;
		//Set update interval
		else if(arg[0..2] == "-i")
		{
			try { upd_interval=to!uint(arg[2..arg.length]);	}
			catch(ConvException excp) { writeln("Wrong interval"); return(1); }
		}
		//Set format string
		else if(arg[0..2] == "-s")
		{
			status_string = arg[2..arg.length];
		}
		//Help
		else if(arg[0..2] == "-h")
		{
			writeln("Usage: ",args[0]," [OPTIONS]\n\n",
					"Options:\n",
					"\t-d\tprint status string to stdout\n",
					"\t-i\tset update interval (in seconds)\n",
					"\t-s\tset status string\n\n",
					"Status string format:\n",
					"\t%T\ttime (12:52)\n",
					"\t%D\tdate (02.03.2011)\n\n",
					"Example:\n",
			 		"\t",args[0]," -d -i1 -s\"| Time: %T | Date: %D |\"");
			return(0);
		}
	}

	main_loop(upd_interval,status_string);

	return(0);
}
