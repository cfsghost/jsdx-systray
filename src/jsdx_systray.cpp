#include <v8.h>
#include <node.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <pthread.h>
#include <ev.h>
#include <string.h>
#include <list>

#include "jsdx_systray.hpp"
#include "ewmh.hpp"
#include "xembed.hpp"
#include "x11.hpp"

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
		SYSTRAY_EVENT_ADD_CLIENT,
		SYSTRAY_EVENT_REMOVE_CLIENT,
		SYSTRAY_EVENT_MAP_CLIENT,
		SYSTRAY_EVENT_UNMAP_CLIENT
	};

	enum {
		SYSTRAY_EVENT_BUTTON_PRESS,
		SYSTRAY_EVENT_BUTTON_RELEASE,
		SYSTRAY_EVENT_MOTION
	};

	/* Event handlers */
	NodeCallback *add_client_cb = NULL;
	NodeCallback *remove_client_cb = NULL;
	NodeCallback *map_client_cb = NULL;
	NodeCallback *unmap_client_cb = NULL;

	/* X11 Atoms */
	Atom a_NET_SYSTEM_TRAY_OPCODE;
	Atom a_NET_SYSTEM_TRAY_MESSAGE_DATA;
	Atom a_NET_SYSTEM_TRAY_REQUEST_DOCK;
	Atom a_NET_SYSTEM_TRAY_BEGIN_MESSAGE;
	Atom a_NET_SYSTEM_TRAY_CANCEL_MESSAGE;
	Atom a_NET_SYSTEM_TRAY_ORIENTATION;
	Atom a_NET_SYSTEM_TRAY_VISUAL;
	Atom a_NET_ACTIVE_WINDOW;
	Atom a_WM_PROTOCOLS;
	Atom a_WM_TAKE_FOCUS;

	/* Thread for X11 event dispatcher */
	static uv_async_t uv_x11event_notifier;
	static uv_check_t uv_x11event_checker;

	Display *display;
	XEvent x11event;

	/* X11 systray */
	Colormap colormap;
	Window trayWindow;
	list<int> trayClients;

	static void X11EventChecker(uv_check_t* handle, int status)
	{
		if (XPending(display))
			uv_async_send(&uv_x11event_notifier);
	}

	static void X11EventCallback(uv_async_t *handle, int status)
	{
		HandleScope scope;

		/* Handle all events */
		while(XPending(display)) {
			XNextEvent(display, &x11event);

			if (x11event.type == DestroyNotify) {
				XDestroyWindowEvent *xewe = (XDestroyWindowEvent *)&x11event;

				if (X11TrayClientExists(xewe->window)) {
					if (remove_client_cb) {
						/* Prepare arguments */
						Local<Value> argv[1] = {
							Local<Value>::New(Integer::New(xewe->window))
						};

						/* Call callback function */
						remove_client_cb->cb->Call(remove_client_cb->Holder, 1, argv);
					}

					/* Remove client of tray */
					trayClients.remove(xewe->window);
				}

			} else if (x11event.type == ClientMessage) {
				if (x11event.xclient.window == trayWindow) {
					if (!XEMBED::eventHandler(&x11event))
						continue;

					if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_OPCODE) {

						/* Dispatch on the request */
						switch(x11event.xclient.data.l[1]) {
						case SYSTEM_TRAY_REQUEST_DOCK: {
							Window clientWin = x11event.xclient.data.l[2];
							bool support_xembed = XEMBED::isSupported(display, clientWin);

							/* If client exists already, ignore this event. */
							if (X11TrayClientExists(clientWin))
								break;

							/* Ping the window and get its state */
							XWindowAttributes xwa;
							if (!XGetWindowAttributes(display, clientWin, &xwa))
								break;

							/* Add to client list */
							trayClients.push_back(clientWin);

							/* Append client window on tray window */
							XUnmapWindow(display, clientWin);

							/* Fixed window size, because Qt application is so stupid. */
							XResizeWindow(display, clientWin, icon_size_width, icon_size_height);
/*
							XSetWindowAttributes attrs;
							attrs.background_pixel = 0;
							attrs.border_pixel = 0;
							attrs.colormap = colormap;
*/
//							attrs.background_pixmap = None;
//							XChangeWindowAttributes(display, clientWin, CWBackPixmap, &attrs);
							XSetWindowBackground(display, clientWin, 0);
//							XChangeWindowAttributes(display, clientWin, CWBackPixel | CWBorderPixel | CWColormap, &attrs);
//							XChangeWindowAttributes(display, clientWin, CWBackPixel | CWBorderPixel, &attrs);

//							XSetWindowBackground(display, clientWin, 0x00606060);
//							XSetWindowBackgroundPixmap(display, clientWin, ParentRelative);

							XReparentWindow(display, clientWin, trayWindow, 0, 0);
							XSync(display, False);
//							XSetWindowColormap(display, clientWin, colormap);

							XSelectInput(display, clientWin, StructureNotifyMask | PropertyChangeMask);

							/* XEMBED */
							if (support_xembed) {
								XEMBED::sendEmbeddedNotify(display, clientWin, trayWindow);
								XEMBED::setWindowActivate(display, clientWin);
								XEMBED::setFocusIn(display, clientWin);

								if (XEMBED::getMappedState(display, clientWin)) {
									XMapRaised(display, clientWin);
								}
							} else {
								XMapRaised(display, clientWin);
							}

							if (add_client_cb) {
								/* Prepare arguments */
								Local<Value> argv[1] = {
									Local<Value>::New(Integer::New(clientWin))
								};

								/* Call callback function */
								add_client_cb->cb->Call(add_client_cb->Holder, 1, argv);
							}

							break;
						}
						case SYSTEM_TRAY_BEGIN_MESSAGE:
							printf("BEGIN_MESSAGE\n");

							break;
						case SYSTEM_TRAY_CANCEL_MESSAGE:
							printf("CANCEL_MESSAGE\n");

							break;
						}

					} else if (x11event.xclient.message_type == a_NET_SYSTEM_TRAY_MESSAGE_DATA) {
						printf("_NET_SYSTEM_TRAY_MESSAGE_DATA\n");
					} else if (x11event.xclient.message_type == a_WM_PROTOCOLS && x11event.xclient.data.l[0] == a_WM_TAKE_FOCUS) {
						printf("WM_TAKE_FOCUS\n");
						/* TODO: Take focus */
					}
				}
			} else if (x11event.type == PropertyNotify) {

				if (X11TrayClientExists(x11event.xproperty.window)) {

					if (!XEMBED::propertyNotifyHandler(&x11event))
						continue;
				}

			} else if (x11event.type == MapNotify) {

				if (X11TrayClientExists(x11event.xmap.window)) {

					if (map_client_cb) {
						/* Prepare arguments */
						Local<Value> argv[1] = {
							Local<Value>::New(Integer::New(x11event.xmap.window))
						};

						/* Call callback function */
						map_client_cb->cb->Call(map_client_cb->Holder, 1, argv);
					}
				}

			} else if (x11event.type == UnmapNotify) {

				if (X11TrayClientExists(x11event.xunmap.window)) {

					if (unmap_client_cb) {
						/* Prepare arguments */
						Local<Value> argv[1] = {
							Local<Value>::New(Integer::New(x11event.xunmap.window))
						};

						/* Call callback function */
						unmap_client_cb->cb->Call(unmap_client_cb->Holder, 1, argv);
					}
				}
			}
		}
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
#if 1
		/* Support ARGB visual */
		int eventBase, errorBase;
		Visual *visual = DefaultVisual(disp, 0);

		if (XRenderQueryExtension(disp, &eventBase, &errorBase)) {
			int i;
			int nvi;
			XVisualInfo xvi_tpl;
			xvi_tpl.screen = screen;
			xvi_tpl.depth = 32;
			xvi_tpl.c_class = TrueColor;

			XVisualInfo *xvi = XGetVisualInfo(disp, VisualScreenMask | VisualDepthMask | VisualClassMask, &xvi_tpl, &nvi);

			/* Set ARGB Mask */
			for (i = 0; i < nvi; ++i) {
				XRenderPictFormat *format = XRenderFindVisualFormat(disp, xvi[i].visual);

				if (format->type == PictTypeDirect && format->direct.alphaMask) {
					visual = xvi[i].visual;

					break;
				}
			}

			XFree(xvi);
		}
#endif
		/* Get Atom which relates to systray */
		char selection_name[32];
		sprintf(selection_name, "_NET_SYSTEM_TRAY_S%d", screen);
		Atom xa_tray_selection = XInternAtom(disp, selection_name, False);

		a_NET_SYSTEM_TRAY_OPCODE = XInternAtom(disp, "_NET_SYSTEM_TRAY_OPCODE", False);
		a_NET_SYSTEM_TRAY_MESSAGE_DATA = XInternAtom(disp, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
		a_NET_SYSTEM_TRAY_ORIENTATION = XInternAtom(disp, "_NET_SYSTEM_TRAY_ORIENTATION", False);
		a_NET_SYSTEM_TRAY_VISUAL = XInternAtom(disp, "_NET_SYSTEM_TRAY_VISUAL", False);

		/* Other X11 Atoms */
		a_NET_ACTIVE_WINDOW = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
		a_WM_PROTOCOLS = XInternAtom(disp, "WM_PROTOCOLS", False);
		a_WM_TAKE_FOCUS = XInternAtom(disp, "WM_TAKE_FOCUS", False);

		/* Listen to X Server */
		uv_check_init(uv_default_loop(), &uv_x11event_checker);
		uv_check_start(&uv_x11event_checker, X11EventChecker);
		uv_async_init(uv_default_loop(), &uv_x11event_notifier, X11EventCallback);

#if 1
		/* Create an invisible window to manage selection */
		XSetWindowAttributes attrs;
		attrs.background_pixel = 0;
		attrs.border_pixel = 0;
		colormap = attrs.colormap = XCreateColormap(disp, root, visual, AllocNone);
#endif
/*
		trayWindow = XCreateWindow(disp, root,
			0, 0,
			-1, -1,
			0, 0,
			InputOnly, DefaultVisual(disp, 0), 0, NULL);
*/
#if 0
		trayWindow = XCreateWindow(disp, root,
			0, 0,
			320, 320,
			0, 32,
			InputOutput, visual, CWBackPixel | CWBorderPixel | CWColormap, &attrs);
#endif
#if 1
		trayWindow = XCreateSimpleWindow(disp, root,
			0, 0, 1, 1,
			0, 0, 0);
#endif
//		XSetWindowBackgroundPixmap(display, trayWindow, 0);
//		XSetWindowBackground(display, trayWindow, 0);
		XSelectInput(disp, trayWindow, StructureNotifyMask | PropertyChangeMask);

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
		//XLowerWindow(disp, trayWindow);
		XRaiseWindow(disp, trayWindow);

		/* Set the orientation */
		unsigned long data = SYSTEM_TRAY_ORIENTATION_HORZ;
		XChangeProperty(disp, trayWindow,
			a_NET_SYSTEM_TRAY_ORIENTATION,
			XA_CARDINAL, 32,
			PropModeReplace,
			(unsigned char *) &data, 1);

		/* Set the visual */
		VisualID visualID = XVisualIDFromVisual(visual);
		XChangeProperty(disp, trayWindow,
			a_NET_SYSTEM_TRAY_VISUAL,
			XA_CARDINAL, 32,
			PropModeReplace,
			(unsigned char *) &visualID, 1);

		/* Set selection owner */
		XSetSelectionOwner(disp, xa_tray_selection, trayWindow, CurrentTime);

		/* Send X event to become selection owner */
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

		return scope.Close(Boolean::New(True));
	}

	static Handle<Value> X11ReleaseSelection(const Arguments& args)
	{
		HandleScope scope;
		list<int>::iterator it;
		Window w;
		int screen = DefaultScreen(display);
		Window root = DefaultRootWindow(display);

		XSelectInput(display, trayWindow, NoEventMask);

		for (it = trayClients.begin(); it != trayClients.end(); it++) {
			w = *it;

			/* Release systray icons */
			XSelectInput(display, w, NoEventMask);
			XUnmapWindow(display, w);
			XReparentWindow(display, w, root, 0, 0);
		}
		XFlush(display);

		/* Destroy tray window */
		trayClients.clear();
		XDestroyWindow(display, trayWindow);

		uv_check_stop((uv_check_t *)&uv_x11event_checker);
		uv_close((uv_handle_t *)&uv_x11event_checker, NULL);

		return Undefined();
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

	static bool X11TrayClientExists(Window w)
	{
		HandleScope scope;
		list<int>::iterator it;

		for (it = trayClients.begin(); it != trayClients.end(); it++) {
			if (w == *it)
				return True;
		}

		return False;
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

			case SYSTRAY_EVENT_REMOVE_CLIENT:
				if (remove_client_cb) {
					remove_client_cb->Holder.Dispose();
					remove_client_cb->cb.Dispose();
				} else {
					remove_client_cb = new NodeCallback();
				}

				/* Set function */
				remove_client_cb->Holder = Persistent<Object>::New(args.Holder());
				remove_client_cb->cb = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
				break;

			case SYSTRAY_EVENT_MAP_CLIENT:
				if (map_client_cb) {
					map_client_cb->Holder.Dispose();
					map_client_cb->cb.Dispose();
				} else {
					map_client_cb = new NodeCallback();
				}

				/* Set function */
				map_client_cb->Holder = Persistent<Object>::New(args.Holder());
				map_client_cb->cb = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
				break;

			case SYSTRAY_EVENT_UNMAP_CLIENT:
				if (unmap_client_cb) {
					unmap_client_cb->Holder.Dispose();
					unmap_client_cb->cb.Dispose();
				} else {
					unmap_client_cb = new NodeCallback();
				}

				/* Set function */
				unmap_client_cb->Holder = Persistent<Object>::New(args.Holder());
				unmap_client_cb->cb = Persistent<Function>::New(Handle<Function>::Cast(args[1]));
				break;
			}
		}

		return Undefined();
	}

	static Handle<Value> X11SendEvent(const Arguments& args)
	{
		HandleScope scope;

		if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsObject()) {

			Window w = args[0]->ToNumber()->Value();
			int eventType = args[1]->ToInteger()->Value();

			if (eventType == SYSTRAY_EVENT_MOTION) {

				XMotionEvent xme;
				xme.display = display;
				xme.time = CurrentTime;
				xme.same_screen = True;
				xme.type = MotionNotify;
				xme.x = args[2]->ToObject()->Get(String::NewSymbol("x"))->ToInteger()->Value();
				xme.y = args[2]->ToObject()->Get(String::NewSymbol("y"))->ToInteger()->Value();

				/* Getting top of window */
				xme.subwindow = args[0]->ToNumber()->Value();
				while(xme.subwindow) {
					xme.window = xme.subwindow;

					XQueryPointer(display, xme.window, &xme.root, &xme.subwindow, &xme.x_root, &xme.y_root, &xme.x, &xme.y, &xme.state);
				}

				XSendEvent(display, xme.window, True, PointerMotionMask, (XEvent *) &xme);
				XFlush(display);


			} else if (eventType == SYSTRAY_EVENT_BUTTON_PRESS || eventType == SYSTRAY_EVENT_BUTTON_RELEASE) {

				XButtonEvent xev;

				xev.display = display;
				xev.time = CurrentTime;
				xev.same_screen = True;
				xev.type = (eventType == SYSTRAY_EVENT_BUTTON_PRESS) ? ButtonPress : ButtonRelease;

				xev.x = args[2]->ToObject()->Get(String::NewSymbol("x"))->ToInteger()->Value();
				xev.y = args[2]->ToObject()->Get(String::NewSymbol("y"))->ToInteger()->Value();

				/* Getting top of window */
				xev.subwindow = w;
				while(xev.subwindow) {
					xev.window = xev.subwindow;

					XQueryPointer(display, xev.window, &xev.root, &xev.subwindow, &xev.x_root, &xev.y_root, &xev.x, &xev.y, &xev.state);
				}

//				XEMBED::setFocusIn(display, trayWindow);
//				XEMBED::setWindowActivate(display, trayWindow);

#if 0
			/* Set focus on window */
			int screen = DefaultScreen(display);

			EWMH::sendClientMsg32(display, root, trayWindow, a_NET_ACTIVE_WINDOW, 1, CurrentTime, 0, 0, 0);
			EWMH::sendClientMsg32(display, root, trayWindow, a_WM_PROTOCOLS, a_WM_TAKE_FOCUS, 0, 0, 0, 0);
//			EWMH::sendClientMsg32(display, root, trayWindow, a_WM_PROTOCOLS, a_WM_TAKE_FOCUS, 0, 0, 0, 0);

			XChangeProperty(display, trayWindow,
				a_NET_ACTIVE_WINDOW, XA_WINDOW, 32, PropModeReplace,
				(unsigned char *)&trayWindow, 1);

//			XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
			XFlush(display);
#endif

				/* Get button which was triggered */
				int button = args[2]->ToObject()->Get(String::NewSymbol("button"))->ToInteger()->Value();
				switch(button) {
				case 1:
					xev.state = Button1Mask;
					xev.button = Button1;
					break;
				case 2:
					xev.state = Button2Mask;
					xev.button = Button2;
					break;
				default:
					xev.state = Button3Mask;
					xev.button = Button3;
				}

				if (eventType == SYSTRAY_EVENT_BUTTON_PRESS) {
					XSendEvent(display, w, True, ButtonPressMask, (XEvent *) &xev);
				} else {
					XSendEvent(display, w, True, ButtonReleaseMask, (XEvent *) &xev);
				}
				XFlush(display);

				Window root = DefaultRootWindow(display);
				X11::setActive(display, w, root, True);
				XFlush(display);
			}
		}

		return Undefined();
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "x11HasSelectionOwner", X11HasSelectionOwner);
		NODE_SET_METHOD(target, "x11AcquireSelection", X11AcquireSelection);
		NODE_SET_METHOD(target, "x11ReleaseSelection", X11ReleaseSelection);
		NODE_SET_METHOD(target, "x11GetTrayClients", X11GetTrayClients);
		NODE_SET_METHOD(target, "x11SendEvent", X11SendEvent);
		NODE_SET_METHOD(target, "on", SetEventHandler);

		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_ADD_CLIENT", SYSTRAY_EVENT_ADD_CLIENT);
		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_REMOVE_CLIENT", SYSTRAY_EVENT_REMOVE_CLIENT);
		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_MAP_CLIENT", SYSTRAY_EVENT_MAP_CLIENT);
		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_UNMAP_CLIENT", SYSTRAY_EVENT_UNMAP_CLIENT);

		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_BUTTON_PRESS", SYSTRAY_EVENT_BUTTON_PRESS);
		JSDX_SYSTRAY_DEFINE_CONSTANT(target, "EVENT_BUTTON_RELEASE", SYSTRAY_EVENT_BUTTON_RELEASE);
	}

	NODE_MODULE(jsdx_systray, init);
}
