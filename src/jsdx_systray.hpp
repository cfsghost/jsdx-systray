#ifndef JSDX_SYSTRAY_H_
#define JSDX_SYSTRAY_H_

#include <v8.h>

namespace JSDXSystray {

#define JSDX_SYSTRAY_DEFINE_CONSTANT(target, name, constant)					\
	(target)->Set(v8::String::NewSymbol(name),							\
	v8::Integer::New(constant),											\
	static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

	struct NodeCallback {
		v8::Persistent<v8::Object> Holder;
		v8::Persistent<v8::Function> cb;

		~NodeCallback() {
			Holder.Dispose();
			cb.Dispose();
		}
	};

	typedef struct {
		long flags;
		long functions;
		long decorations;
		long input_mode;
		long state;
	} MotifWmHints;

	static v8::Handle<v8::Value> X11GetTrayClients(const v8::Arguments& args);
	static bool X11TrayClientExists(Window w);
}

#endif
