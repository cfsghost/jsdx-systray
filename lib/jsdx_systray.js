var _systray = require('../build/Release/jsdx_systray');

var Systray = module.exports = function() {
};

Systray.prototype.getSelectionOwner = function() {

	console.log(_systray.x11GetSelectionOwner());
};
