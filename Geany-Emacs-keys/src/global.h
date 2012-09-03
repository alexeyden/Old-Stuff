#ifndef GLOBAL_H
#define GLOBAL_H

GeanyPlugin         *geany_plugin;
GeanyData           *geany_data;
GeanyFunctions      *geany_functions;

void call_func(guint num);

//for repeat command
guint repeat_count;

#endif
