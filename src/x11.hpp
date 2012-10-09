#ifndef JSDX_JSDX_SYSTRAY_X11_H_
#define JSDX_JSDX_SYSTRAY_X11_H_

namespace JSDXSystray {

	namespace X11 {

		void setActive(Display *disp, Window root, Window w, bool active);
		void send_xclient_message(Display *disp, Window w, Atom a, long data0, long data1);

	}

}

#endif
