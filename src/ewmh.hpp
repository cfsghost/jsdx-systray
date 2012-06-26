#ifndef JSDX_JSDX_SYSTRAY_EWMH_H_
#define JSDX_JSDX_SYSTRAY_EWMH_H_

#define FAILURE 0

namespace JSDXSystray {

	namespace EWMH {

		int sendClientMsg32(Display *disp, Window root, Window w, Atom type, long data0, long data1, long data2, long data3, long data4) ;
		int setWindowState(Display *disp, Window w, const char *state);
	}

}

#endif
