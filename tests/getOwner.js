var Systray = require('../');

var systray = new Systray;
if (systray.hasSelectionOwner()) {
	systray.acquireSelection();
}
