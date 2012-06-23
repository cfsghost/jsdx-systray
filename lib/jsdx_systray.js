var _systray = require('../build/Release/jsdx_systray');

var Systray = module.exports = function() {
};

Systray.prototype.EVENT_ADD_CLIENT = _systray.EVENT_ADD_CLIENT;

Systray.prototype.hasSelectionOwner = function() {
	return _systray.x11HasSelectionOwner();
};

Systray.prototype.acquireSelection = function() {
	return _systray.x11AcquireSelection();
};

Systray.prototype.getTrayClients = function() {
	return _systray.x11GetTrayClients();
};

Systray.prototype.on = function(e, callback) {
	_systray.on.apply(this, [ e, callback]);
};
