#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.hpp"
#include "xembed.hpp"

namespace JSDXSystray {

	void XEMBED::sendEmbeddedNotify(Display *display, Window dest, Window embedder)
	{
		Atom a_XEMBED = XInternAtom(display, "_XEMBED", False);

		EWMH::sendClientMsg32(display, dest, dest, a_XEMBED, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0, embedder, 0);
	}

	void XEMBED::sendGrabNotify(Display *display, Window dest)
	{
		Atom a_XEMBED = XInternAtom(display, "_XEMBED", False);

		EWMH::sendClientMsg32(display, dest, dest, a_XEMBED, CurrentTime, XEMBED_MODALITY_ON, 0, 0, 0);
	}

	void XEMBED::setWindowActivate(Display *display, Window dest)
	{
		Atom a_XEMBED = XInternAtom(display, "_XEMBED", False);

		EWMH::sendClientMsg32(display, dest, dest, a_XEMBED, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);
	}

	void XEMBED::setFocusIn(Display *display, Window dest)
	{
		Atom a_XEMBED = XInternAtom(display, "_XEMBED", False);

		EWMH::sendClientMsg32(display, dest, dest, a_XEMBED, CurrentTime, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT, 0, 0);
	}

	bool XEMBED::getMappedState(Display *display, Window dest)
	{
		unsigned long data0, data1;

		if (XEMBED::getInfo(display, dest, &data0, &data1)) {
			return ((data1 && XEMBED_MAPPED) != 0);
		}

		return False;
	}

	bool XEMBED::isSupported(Display *display, Window dest)
	{
		unsigned long data0, data1;

		if (XEMBED::getInfo(display, dest, &data0, &data1)) {
			return True;
		}

		return False;
	}

	bool XEMBED::getInfo(Display *display, Window dest, unsigned long *data0, unsigned long *data1)
	{
		Atom a_XEMBED_INFO = XInternAtom(display, "_XEMBED_INFO", False);
		Atom act_type;
		int act_fmt;
		unsigned long nitems, bytesafter, *data;
		int rc;

		rc = XGetWindowProperty(display, dest,
			a_XEMBED_INFO, 0, 2, False, a_XEMBED_INFO,
			&act_type, &act_fmt, &nitems,
			&bytesafter, (unsigned char **)&data);

		if (rc != Success)
			return False;

		if (act_type == a_XEMBED_INFO && nitems == 2) {
			*data0 = data[0];
			*data1 = data[1];
			return True;
		}

		return False;
	}

	bool XEMBED::eventHandler(XEvent *xev)
	{
		Atom a_XEMBED = XInternAtom(xev->xclient.display, "_XEMBED", False);

		if (xev->xclient.message_type == a_XEMBED) {
			switch(xev->xclient.data.l[1]) {
			case XEMBED_EMBEDDED_NOTIFY:
				printf("XEMBED_EMBEDDED_NOTIFY\n");
				break;
			case XEMBED_WINDOW_ACTIVATE:
				printf("XEMBED_WINDOW_ACTIVATE\n");
				break;
			case XEMBED_WINDOW_DEACTIVATE:
			case XEMBED_MODALITY_ON:
			case XEMBED_MODALITY_OFF:
			case XEMBED_FOCUS_IN:
				printf("XEMBED_FOCUS_IN\n");
				break;
			case XEMBED_FOCUS_OUT:
				printf("XEMBED_FOCUS_OUT\n");
				break;

			case XEMBED_REQUEST_FOCUS:
				printf("XEMBED_REQUEST_FOCUS\n");
				//gtk_socket_claim_focus (socket, TRUE);
				break;

			case XEMBED_FOCUS_NEXT:
			case XEMBED_FOCUS_PREV:
				printf("XEMBED_REQUEST_PREV\n");
//				gtk_socket_advance_toplevel_focus (socket,
//					(message == XEMBED_FOCUS_NEXT ?
//					GTK_DIR_TAB_FORWARD : GTK_DIR_TAB_BACKWARD));
				break;
/*
			case XEMBED_GTK_GRAB_KEY:
				gtk_socket_add_grabbed_key (socket, data1, data2);
				break;
			case XEMBED_GTK_UNGRAB_KEY:
				gtk_socket_remove_grabbed_key (socket, data1, data2);
			break;
			case XEMBED_GRAB_KEY:
			case XEMBED_UNGRAB_KEY:
			break;
*/

			default:
				printf("UNKNOWN _XEMBED message\n");
//				GTK_NOTE (PLUGSOCKET,
//					g_message ("GtkSocket: Ignoring unknown _XEMBED message of type %d", message));
				break;
			}

			printf("XEMBED!!\n");
			return False;
		}

		return True;
	}

	bool XEMBED::propertyNotifyHandler(XEvent *xev)
	{
		Atom a_XEMBED = XInternAtom(xev->xproperty.display, "_XEMBED", False);

		if (xev->xproperty.atom == a_XEMBED) {
			printf("XEMBED PROPERTY!!\n");

			if (XEMBED::getMappedState(xev->xproperty.display, xev->xproperty.window)) {
				XMapRaised(xev->xproperty.display, xev->xproperty.window);
			}

			return False;
		}

		return True;
	}
}
