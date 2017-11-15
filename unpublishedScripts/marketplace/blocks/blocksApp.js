///
/// blocksApp.js
/// A tablet app for downloading 3D assets from Google Blocks
/// 
/// Author: Elisa Lupin-Jimenez
/// Copyright High Fidelity 2017
///
/// Licensed under the Apache 2.0 License
/// See accompanying license file or http://apache.org/
///
/// All assets are under CC Attribution Non-Commerical
/// http://creativecommons.org/licenses/
///

(function () {
	var APP_NAME = "BLOCKS";
	var APP_URL = "https://vr.google.com/objects/";
	var APP_OUTDATED_URL = "https://hifi-content.s3.amazonaws.com/elisalj/blocks/updateToBlocks.html";
	var APP_ICON = "https://hifi-content.s3.amazonaws.com/elisalj/blocks/blocks-i.svg";
	var APP_ICON_ACTIVE = "https://hifi-content.s3.amazonaws.com/elisalj/blocks/blocks-a.svg";

    try {
        print("Current Interface version: " + Window.checkVersion());
    } catch(err) {
        print("Outdated Interface version does not support Blocks");
        APP_URL = APP_OUTDATED_URL;
    }

	var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
	var button = tablet.addButton({
	    icon: APP_ICON,
        activeIcon: APP_ICON_ACTIVE,
		text: APP_NAME
	});

	function onClicked() {
	    if (!shown) {
	        tablet.gotoWebScreen(APP_URL, "", true);
	    } else {
	        tablet.gotoHomeScreen();
	    }
	}
	button.clicked.connect(onClicked);

	var shown = false;

	function checkIfBlocks(url) {
	    if (url.indexOf("google") !== -1) {
	        return true;
	    }
	    return false;
	}

	function onScreenChanged(type, url) {
	    if ((type === 'Web' && checkIfBlocks(url)) || url === APP_OUTDATED_URL) {
	        button.editProperties({ isActive: true });
	        shown = true;
	    } else {
	        button.editProperties({ isActive: false });
	        shown = false;
	    }
	}

	tablet.screenChanged.connect(onScreenChanged);

	function cleanup() {
	    tablet.removeButton(button);
	}

	Script.scriptEnding.connect(cleanup);
}());
