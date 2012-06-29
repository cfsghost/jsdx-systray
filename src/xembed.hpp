#ifndef JSDX_JSDX_SYSTRAY_XEMBED_H_
#define JSDX_JSDX_SYSTRAY_XEMBED_H_

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6   
#define XEMBED_FOCUS_PREV               7
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

/* Details for XEMBED_FOCUS_IN */
#define XEMBED_FOCUS_CURRENT            0
#define XEMBED_FOCUS_FIRST              1
#define XEMBED_FOCUS_LAST               2

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                   (1 << 0)

namespace JSDXSystray {

	namespace XEMBED {

		void sendEmbeddedNotify(Display *display, Window dest, Window embedder);
		void sendGrabNotify(Display *display, Window dest);
		void setWindowActivate(Display *display, Window dest);
		void setFocusIn(Display *display, Window dest);
		bool getMappedState(Display *display, Window dest);
		bool isSupported(Display *display, Window dest);
		bool getInfo(Display *display, Window dest, unsigned long *data0, unsigned long *data1);
		bool eventHandler(XEvent *xev);
		bool propertyNotifyHandler(XEvent *xev);

	}

}

#endif
