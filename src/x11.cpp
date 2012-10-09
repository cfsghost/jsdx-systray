#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "x11.hpp"

namespace JSDXSystray {

	void X11::setActive(Display *disp, Window root, Window w, bool active)
	{
		Atom atom = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
		Atom wm_protocol = XInternAtom(disp, "WM_PROTOCOLS", False);
		Atom wm_take_focus = XInternAtom(disp, "WM_TAKE_FOCUS", False);

		if (active) {
			send_xclient_message(disp, w, wm_protocol, wm_take_focus, 0L);

			XChangeProperty(disp, root,
				atom, XA_WINDOW, 32, PropModeReplace,
				(unsigned char *)&w, 1);
		} else {
			XDeleteProperty(disp, root, atom);
		}
	}

	void X11::send_xclient_message(Display *disp, Window w, Atom a, long data0, long data1)
	{
		XEvent ev;

		memset(&ev, 0, sizeof(ev));
		ev.xclient.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = a;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = data0;
		ev.xclient.data.l[1] = data1;

		XSendEvent(disp, w, False, 0L, (XEvent *) &ev);
	}
}
