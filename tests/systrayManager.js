var Systray = require('../');

var systray = new Systray;
if (systray.hasSelectionOwner()) {
	systray.acquireSelection();

	systray.on(systray.EVENT_ADD_CLIENT, function(w) {
		console.log(w);
	});
}
