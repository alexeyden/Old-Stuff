/* Set WMFS status directly using X */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

unsigned int wmfs_set_status(char* text)
{
	Display* dpy = XOpenDisplay(NULL);
	if(!dpy)
		return 0;
	
	/* TODO: for all screens */
	char* atom_wmfs = "_WMFS_STATUSTEXT_0";
	XChangeProperty(dpy,
					RootWindow(dpy,DefaultScreen(dpy)),
					XInternAtom(dpy,atom_wmfs,False),
					XInternAtom(dpy,"UTF8_STRING",False),
					8,
					PropModeReplace,
					(unsigned char*)text,
					strlen(text));
	
	XClientMessageEvent e;
	e.type = ClientMessage;
	e.message_type = XInternAtom(dpy,atom_wmfs,False);
	e.window = RootWindow(dpy,DefaultScreen(dpy));
	e.format = 32;
	e.data.l[0]=0;
	e.data.l[1]=0;
	e.data.l[2]=0;
	e.data.l[3]=0;
	e.data.l[4]=True;
	
	XSendEvent(dpy,RootWindow(dpy,DefaultScreen(dpy)),False,StructureNotifyMask,(XEvent*)&e);
	XSync(dpy,False);

	XCloseDisplay(dpy);
	return 1;
}

unsigned int wmfs_is_running()
{
	Atom rt;
	int rf;
	unsigned long ir, il;
	unsigned char *ret;

	Display* dpy = XOpenDisplay(NULL);
	if(!dpy)
		return 0;
	
	XGetWindowProperty(dpy, RootWindow(dpy,DefaultScreen(dpy)),
						XInternAtom(dpy,"_WMFS_RUNNING",False),
						0L, 4096, False, XA_CARDINAL, &rt, &rf, &ir, &il, &ret);
	
	if(!ret)
	{
		XFree(ret);
		XCloseDisplay(dpy);
		return 0;
	}

	XFree(ret);
	XCloseDisplay(dpy);

	return 1;
}
