#include <v8.h>
#include <node.h>
#include <X11/Xlib.h>
#include <pthread.h>
#include <ev.h>
#include <string.h>

#include "jsdx_systray.hpp"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

namespace JSDXSystray {
 
	using namespace node;
	using namespace v8;

	Atom a_NET_SYSTEM_TRAY_OPCODE;
	Atom a_NET_SYSTEM_TRAY_MESSAGE_DATA;
	Atom a_NET_SYSTEM_TRAY_REQUEST_DOCK;
	Atom a_NET_SYSTEM_TRAY_BEGIN_MESSAGE;
	Atom a_NET_SYSTEM_TRAY_CANCEL_MESSAGE;

	static pthread_t x11event_thread;
	static ev_async eio_x11event_notifier;

	XEvent x11event;
	pthread_mutex_t x11event_mutex = PTHREAD_MUTEX_INITIALIZER;

	static void *X11EventDispatcherThread(void *)
	{
		Display *disp = XOpenDisplay(NULL);

		while(1) {
			if (XPending(disp)) {

				printf("12313123\n");

				/* Got event */
				pthread_mutex_lock(&x11event_mutex);
				XNextEvent(disp, &x11event);
				pthread_mutex_unlock(&x11event_mutex);

				ev_async_send(EV_DEFAULT_UC_ &eio_x11event_notifier);
			}
		}
	}

	static void X11EventCallback(EV_P_ ev_async *watcher, int revents)
	{
		HandleScope scope;

		assert(watcher == &eio_x11event_notifier);
		assert(revents == EV_ASYNC);

		pthread_mutex_lock(&x11event_mutex);

		if (x11event.type == ClientMessage) {
			if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_OPCODE) {
				printf("_NET_SYSTEM_TRAY_OPCODE\n");

				/* Dispatch on the request */
				switch(x11event.xclient.data.l[1]) {
				case SYSTEM_TRAY_REQUEST_DOCK:

					break;
				case SYSTEM_TRAY_BEGIN_MESSAGE:

					break;
				case SYSTEM_TRAY_CANCEL_MESSAGE:

					break;
				}
			} else if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_MESSAGE_DATA) {
				printf("_NET_SYSTEM_TRAY_MESSAGE_DATA\n");
			}
		}

		pthread_mutex_unlock(&x11event_mutex);

	}

	static Handle<Value> X11HasSelectionOwner(const Arguments& args)
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

	static Handle<Value> X11AcquireSelection(const Arguments& args)
	{
		HandleScope scope;

		/* Get current display, screen and root window */
		Display *disp = XOpenDisplay(NULL);
		int screen = DefaultScreen(disp);
		Window root = DefaultRootWindow(disp);

		/* Get Atom */
		char selection_name[32];
		sprintf(selection_name, "_NET_SYSTEM_TRAY_S%d", screen);
		Atom xa_tray_selection = XInternAtom(disp, selection_name, False);

		a_NET_SYSTEM_TRAY_OPCODE = XInternAtom(disp, "_NET_SYSTEM_TRAY_OPCODE", False);
		a_NET_SYSTEM_TRAY_MESSAGE_DATA = XInternAtom(disp, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);

		/* Create an invisible window to manage selection */
		Window w = XCreateWindow(disp, root,
			0, 0,
			-1, -1,
			0, 0,
			InputOnly, DefaultVisual(disp, 0), 0, NULL);

		/* Set selection owner */
		XSetSelectionOwner(disp, xa_tray_selection, w, CurrentTime);

		/* Send X event to become an owner */
		XClientMessageEvent xev;
		xev.type = ClientMessage;
		xev.window = root;
		xev.message_type = XInternAtom(disp, "MANAGER", False);
		xev.format = 32;
		xev.data.l[0] = CurrentTime;
		xev.data.l[1] = xa_tray_selection;
		xev.data.l[2] = w;
		xev.data.l[3] = 0;
		xev.data.l[4] = 0;
		XSendEvent(disp, root, False, StructureNotifyMask, (XEvent *) &xev);

		/* start background thread and event handler for callback */
		ev_async_init(&eio_x11event_notifier, X11EventCallback);
		ev_async_start(EV_DEFAULT_UC_ &eio_x11event_notifier);

		ev_ref(EV_DEFAULT_UC);

		pthread_create(&x11event_thread, NULL, X11EventDispatcherThread, 0);

		return scope.Close(Boolean::New(True));
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "x11HasSelectionOwner", X11HasSelectionOwner);
		NODE_SET_METHOD(target, "x11AcquireSelection", X11AcquireSelection);
	}

	NODE_MODULE(jsdx_systray, init);
}
