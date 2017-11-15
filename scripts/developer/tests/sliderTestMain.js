(function () {
	var HTML_URL = Script.resolvePath("sliderTest.html");
	var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: "SLIDER"
    });

    function onClicked() {
        tablet.gotoWebScreen(HTML_URL);
    }

    button.clicked.connect(onClicked);

    var onSliderTestScreen = false;
    function onScreenChanged(type, url) {
        if (type === "Web" && url === HTML_URL) {
            // when switching to the slider page, change inputMode to "Mouse", this should make the sliders work.
            onSliderTestScreen = true;
            Overlays.editOverlay(HMD.tabletScreenID, { inputMode: "Mouse" });
        } else if (onSliderTestScreen) {
            // when switching off of the slider page, change inputMode to back to "Touch".
            onSliderTestScreen = false;
            Overlays.editOverlay(HMD.tabletScreenID, { inputMode: "Touch" });
        }
    }

    tablet.screenChanged.connect(onScreenChanged);

	function cleanup() {
        tablet.removeButton(button);
        tablet.screenChanged.disconnect(onScreenChanged);
    }
    Script.scriptEnding.connect(cleanup);

}());
