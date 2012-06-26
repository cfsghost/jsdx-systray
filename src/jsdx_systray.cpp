#include <v8.h>
#include <node.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <pthread.h>
#include <ev.h>
#include <string.h>
#include <list>

#include "jsdx_systray.hpp"
#include "ewmh.hpp"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

namespace JSDXSystray {
 
	using namespace node;
	using namespace v8;
	using namespace std;

	/* Properties */
	int icon_size_width = 32;
	int icon_size_height = 32;

	/* Events */
	enum {
		SYSTRAY_EVENT_ADD_CLIENT
	};

	/* Event handlers */
	NodeCallback *add_client_cb = NULL;

	/* X11 Atoms */
	Atom a_NET_SYSTEM_TRAY_OPCODE;
	Atom a_NET_SYSTEM_TRAY_MESSAGE_DATA;
	Atom a_NET_SYSTEM_TRAY_REQUEST_DOCK;
	Atom a_NET_SYSTEM_TRAY_BEGIN_MESSAGE;
	Atom a_NET_SYSTEM_TRAY_CANCEL_MESSAGE;
	Atom a_NET_SYSTEM_TRAY_ORIENTATION;

	/* Thread for X11 event dispatcher */
	static pthread_t x11event_thread;
	static ev_async eio_x11event_notifier;
	pthread_mutex_t x11event_mutex = PTHREAD_MUTEX_INITIALIZER;
	Display *display;
	XEvent x11event;

	/* X11 systray */
	Window trayWindow;
	list<int> trayClients;

	static void *X11EventDispatcherThread(void *d)
	{
		Display *disp = (Display *)d;
		int fd = ConnectionNumber(disp);
		struct timeval tv;
		fd_set in_fds;

		while(1) {

			/* Create a File Description for X11 display */
			FD_ZERO(&in_fds);
			FD_SET(fd, &in_fds);

			/* Set our timer */
			tv.tv_usec = 0;
			tv.tv_sec = 1;

			/* Wait for X Event or a Timer */
			if (select(fd + 1, &in_fds, 0, 0, &tv)) {

				pthread_mutex_lock(&x11event_mutex);

				/* Call handler if there is events */
				if (XPending(disp)) {
					ev_async_send(EV_DEFAULT_UC_ &eio_x11event_notifier);
				}

				pthread_mutex_unlock(&x11event_mutex);
			}
		}
	}

	static void X11EventCallback(EV_P_ ev_async *watcher, int revents)
	{
		HandleScope scope;

		assert(watcher == &eio_x11event_notifier);
		assert(revents == EV_ASYNC);

		pthread_mutex_lock(&x11event_mutex);

		/* Handle all events */
		while(XPending(display)) {
			XNextEvent(display, &x11event);

			if (x11event.type == DestroyNotify) {
				XDestroyWindowEvent *xewe = (XDestroyWindowEvent *)&x11event;

				/* Remove client of tray */
				trayClients.remove(xewe->window);

			} else if (x11event.type == ClientMessage) {
				if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_OPCODE) {

					/* Dispatch on the request */
					switch(x11event.xclient.data.l[1]) {
					case SYSTEM_TRAY_REQUEST_DOCK:

						if (x11event.xclient.window == trayWindow) {

							/* Add to client list */
							trayClients.push_back(x11event.xclient.data.l[2]);

							/* Fixed window size, because Qt application is so stupid. */
							XResizeWindow(display, x11event.xclient.data.l[2], icon_size_width, icon_size_height);
							XReparentWindow(display, x11event.xclient.data.l[2], trayWindow, 0, 0);

							XMapWindow(display, x11event.xclient.data.l[2]);

							/* Prepare arguments */
							Local<Value> argv[1] = {
								Local<Value>::New(Integer::New(x11event.xclient.data.l[2]))
							};

							/* Call callback function */
							add_client_cb->cb->Call(add_client_cb->Holder, 1, argv);
						}

						break;
					case SYSTEM_TRAY_BEGIN_MESSAGE:
						printf("BEGIN_MESSAGE\n");

						break;
					case SYSTEM_TRAY_CANCEL_MESSAGE:
						printf("CANCEL_MESSAGE\n");

						break;
					}
				} else if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_MESSAGE_DATA) {
					printf("_NET_SYSTEM_TRAY_MESSAGE_DATA\n");
				}
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
		Display *disp = display = XOpenDisplay(NULL);
		int screen = DefaultScreen(disp);
		Window root = DefaultRootWindow(disp);

		/* Get Atom */
		char selection_name[32];
		sprintf(selection_name, "_NET_SYSTEM_TRAY_S%d", screen);
		Atom xa_tray_selection = XInternAtom(disp, selection_name, False);

		a_NET_SYSTEM_TRAY_OPCODE = XInternAtom(disp, "_NET_SYSTEM_TRAY_OPCODE", False);
		a_NET_SYSTEM_TRAY_MESSAGE_DATA = XInternAtom(disp, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
		a_NET_SYSTEM_TRAY_ORIENTATION = XInternAtom(disp, "_NET_SYSTEM_TRAY_ORIENTATION", False);

		/* Create thread */
		pthread_create(&x11event_thread, NULL, X11EventDispatcherThread, (void *)disp);

		/* Create an invisible window to manage selection */
/*
		trayWindow = XCreateWindow(disp, root,
			0, 0,
			-1, -1,
			0, 0,
			InputOnly, DefaultVisual(disp, 0), 0, NULL);
*/
		trayWindow = XCreateSimpleWindow(disp, root,
			0, 0, 1, 1,
			0, 0, 0);

		XSelectInput(disp, trayWindow, StructureNotifyMask | FocusChangeMask | PropertyChangeMask | ExposureMask);

		/* Set tray window states */
		EWMH::setWindowState(disp, trayWindow, "_NET_WM_STATE_STICKY");
		EWMH::setWindowState(disp, trayWindow, "_NET_WM_STATE_SKIP_TASKBAR");
		EWMH::setWindowState(disp, trayWindow, "_NET_WM_STATE_SKIP_PAGER");

		/* Take off tray window decorator */
		MotifWmHints new_hints;
		MotifWmHints *hints;
		Atom hints_atom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
		int format;
		Atom type;
		unsigned long nitems;
		unsigned long bytes_after;
		unsigned char *hints_data;

		XGetWindowProperty(display, trayWindow,
			hints_atom, 0, sizeof(MotifWmHints) / sizeof(long),
			False, AnyPropertyType, &type, &format, &nitems,
			&bytes_after, &hints_data);
		if (type == None) {
			new_hints.flags = (1L << 1);
			new_hints.decorations = 0L;
			hints = &new_hints;
		} else {
			hints = (MotifWmHints *)hints_data;
			hints->flags |= (1L << 1);
			hints->decorations = 0L;
		}

		XGrabServer(display);

		XChangeProperty(display, trayWindow, hints_atom,
			hints_atom, 32, PropModeReplace,
			(unsigned char *)hints, sizeof(MotifWmHints) / sizeof(long));

		if (hints != &new_hints)
			XFree(hints);

		XUngrabServer(display);

		/* Show */
		XMapWindow(disp, trayWindow);

		/* Set selection owner */
		XSetSelectionOwner(disp, xa_tray_selection, trayWindow, CurrentTime);

		/* Send X event to become an owner */
		XClientMessageEvent xev;
		xev.type = ClientMessage;
		xev.window = root;
		xev.message_type = XInternAtom(disp, "MANAGER", False);
		xev.format = 32;
		xev.data.l[0] = CurrentTime;
		xev.data.l[1] = xa_tray_selection;
		xev.data.l[2] = trayWindow;
		xev.data.l[3] = 0;
		xev.data.l[4] = 0;
		XSendEvent(disp, root, False, StructureNotifyMask, (XEvent *) &xev);

		/* Set the orientation */
		unsigned long data = SYSTEM_TRAY_ORIENTATION_HORZ;
		XChangeProperty(disp, trayWindow,
			a_NET_SYSTEM_TRAY_ORIENTATION,
			XA_CARDINAL, 32,
			PropModeReplace,
			(unsigned char *) &data, 1);

		/* start background thread and event handler for callback */
		ev_async_init(&eio_x11event_notifier, X11EventCallback);
		ev_async_start(EV_DEFAULT_UC_ &eio_x11event_notifier);

		ev_ref(EV_DEFAULT_UC);

		return scope.Close(Boolean::New(True));
	}

	static Handle<Value> X11GetTrayClients(const Arguments& args)
	{
		HandleScope scope;
		list<int>::iterator it;
		int i = 0;

		/* Prepare JavaScript array */
		Local<Array> arr = Array::New(trayClients.size()); 

		for (it = trayClients.begin(); it != trayClients.end(); it++, i++) {
			arr->Set(i, Integer::New(*it));
		}

		return scope.Close(arr);
	}

	static Handle<Value> SetEventHandler(const Arguments& args)
	{
		HandleScope scope;

		if (args[0]->IsNumber() && args[1]->IsFunction()) {
			switch(args[0]->ToInteger()->Value()) {
			case SYSTRAY_EVENT_ADD_CLIENT:
				if (add_client_cb) {
					add_client_cb->Holder.Dispose();
					add_client_cb->cb.Dispose();
				} else {
					add_client_cb = new NodeCallback();
				}

				/* Set function */
				add_client_cb->Holder = Persistent<Object>::New(args.Holder());
				add_client_cb->cb = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
				break;
			}
		}

		return Undefined();
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "x11HasSelectionOwner", X11HasSelectionOwner);
		NODE_SET_METHOD(target, "x11AcquireSelection", X11AcquireSelection);
		NODE_SET_METHOD(target, "x11GetTrayClients", X11GetTrayClients);
		NODE_SET_METHOD(target, "on", SetEventHandler);

		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_ADD_CLIENT", SYSTRAY_EVENT_ADD_CLIENT);
	}

	NODE_MODULE(jsdx_systray, init);
}
