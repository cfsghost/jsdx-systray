var _systray = require('../build/Release/jsdx_systray');

var Systray = module.exports = function() {
};

Systray.prototype.EVENT_ADD_CLIENT = _systray.EVENT_ADD_CLIENT;
Systray.prototype.EVENT_REMOVE_CLIENT = _systray.EVENT_REMOVE_CLIENT;
Systray.prototype.EVENT_MAP_CLIENT = _systray.EVENT_MAP_CLIENT;
Systray.prototype.EVENT_UNMAP_CLIENT = _systray.EVENT_UNMAP_CLIENT;

/* Input Events */
Systray.prototype.EVENT_MOTION = _systray.EVENT_MOTION;
Systray.prototype.EVENT_BUTTON_PRESS = _systray.EVENT_BUTTON_PRESS;
Systray.prototype.EVENT_BUTTON_RELEASE = _systray.EVENT_BUTTON_RELEASE;

Systray.prototype.hasSelectionOwner = function() {
	return _systray.x11HasSelectionOwner();
};

Systray.prototype.acquireSelection = function() {
	return _systray.x11AcquireSelection();
};

Systray.prototype.releaseSelection = function() {
	return _systray.x11ReleaseSelection();
};

Systray.prototype.getTrayClients = function() {
	return _systray.x11GetTrayClients();
};

Systray.prototype.sendEvent = function(w, e, args) {
	return _systray.x11SendEvent.apply(this, [ w, e, args ]);
};

Systray.prototype.on = function(e, callback) {
	var self = this;

	var _callback = function() {
		callback.apply(self, Array.prototype.slice.call(arguments));
	};

	_systray.on(e, _callback);
};
