import std.stdio;
import std.datetime;
import std.conv;
import std.string;
import std.array;
import std.file;

extern(C)
{
	static int  get_alsa_volume();
}

string get_time()
{
	auto cur_time = Clock.currTime();
	return std.string.format((cur_time.hour()<10)?"0":"",
							 cur_time.hour(),":",
							 (cur_time.minute()<10)?"0":"",
							 cur_time.minute());
}

string get_date()
{
	auto cur_time = Clock.currTime();
	return std.string.format(cur_time.day(),".",
							 cur_time.month(),".",
							 cur_time.year());
}

string get_volume()
{
	return to!string(get_alsa_volume());
}

string get_battery()
{
	string batt_path = "/sys/class/power_supply/BAT0";

	float bat_energy_now = 1.0;
	float bat_energy_full = 1.0;

	string bt_now_str = cast(string) read(batt_path~"/energy_now");
	string bt_full_str = cast(string) read(batt_path~"/energy_full");
	
	try {
		bat_energy_now = std.conv.parse!(int)(bt_now_str);
		bat_energy_full = std.conv.parse!(int)(bt_full_str);
	}
	catch(FileException fexcp){
		return "N\\A";
	}

	return to!string(roundTo!(int)(bat_energy_now*100/bat_energy_full));
}
