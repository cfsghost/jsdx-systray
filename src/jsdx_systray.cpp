#include <v8.h>
#include <node.h>
#include <X11/Xlib.h>
#include <pthread.h>
#include <ev.h>
#include <string.h>

#include "jsdx_systray.hpp"

namespace JSDXSystray {
 
	using namespace node;
	using namespace v8;

	static Handle<Value> X11GetSelectionOwner(const Arguments& args)
	{
		HandleScope scope;

		/* Get current display and screen */
		Display *disp = XOpenDisplay(NULL);
		int screen = DefaultScreen(disp);
		char selection_name[32];

		sprintf(selection_name, "_NET_SYSTEM_TRAY_S%d", screen);

		Atom xa_tray_selection = XInternAtom(disp, selection_name, False);

		if (XGetSelectionOwner(disp, xa_tray_selection) != None) {
			return scope.Close(Boolean::New(False));
		}

		return scope.Close(Boolean::New(True));
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "x11GetSelectionOwner", X11GetSelectionOwner);
	}

	NODE_MODULE(jsdx_systray, init);
}
