#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.hpp"

namespace JSDXSystray {

	int EWMH::sendClientMsg32(Display *disp, Window root, Window w, Atom type, long data0, long data1, long data2, long data3, long data4) 
	{
		XEvent ev;  
		Status rc; 

		ev.xclient.type = ClientMessage;
		ev.xclient.serial = 0;
		ev.xclient.send_event = True;
		ev.xclient.message_type = type;
		ev.xclient.window = w;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = data0;
		ev.xclient.data.l[1] = data1;
		ev.xclient.data.l[2] = data2;
		ev.xclient.data.l[3] = data3;
		ev.xclient.data.l[4] = data4; 

		rc = XSendEvent(disp, root, False, 0xFFFFFF, &ev);

		return (rc != 0);
	}
 
	int EWMH::setWindowState(Display *disp, Window w, const char *state)
	{
		int ret;
		XWindowAttributes xwa;
		Atom wm_state = XInternAtom(disp, "_NET_WM_STATE", False);
		Atom state_atom = XInternAtom(disp, state, False);

		/* Ping the window and get its state */
		ret = XGetWindowAttributes(disp, w, &xwa);
		if (!ret)
			return FAILURE;

		if (xwa.map_state != IsUnmapped) {
			Atom state_add = XInternAtom(disp, "_NET_WM_STATE_ADD", False);

			sendClientMsg32(disp, xwa.root, w, wm_state, state_add, state_atom, 0, 0, 0);

		} else {
			ret = XChangeProperty(disp, w, wm_state,
				XA_ATOM, 32, PropModeAppend,
				(unsigned char *)&state_atom, 1);
		}

		return ret;
	}
}
