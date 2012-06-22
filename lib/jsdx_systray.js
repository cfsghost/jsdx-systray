var _systray = require('../build/Release/jsdx_systray');

var Systray = module.exports = function() {
};

Systray.prototype.hasSelectionOwner = function() {
	return _systray.x11HasSelectionOwner();
};

Systray.prototype.acquireSelection = function() {
	return _systray.x11AcquireSelection();
};
