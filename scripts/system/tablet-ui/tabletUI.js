"use strict";

//
//  tabletUI.js
//
//  scripts/system/tablet-ui/
//
//  Created by Seth Alves 2016-9-29
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, HMD, WebTablet, UIWebTablet */

(function() { // BEGIN LOCAL_SCOPE
    var tabletShown = false;
    var tabletLocation = null;

    Script.include("../libraries/WebTablet.js");

    function showTabletUI() {
        tabletShown = true;
        print("show tablet-ui");
        UIWebTablet = new WebTablet("qml/hifi/tablet/Tablet.qml", null, null, tabletLocation);
		UIWebTablet.register();
        HMD.tabletID = UIWebTablet.webEntityID;

        var setUpTabletUI = function() {
            var root = UIWebTablet.getRoot();
            if (!root) {
                print("HERE no root yet");
                Script.setTimeout(setUpTabletUI, 100);
                return;
            }
            print("HERE got root", root);
        }

        Script.setTimeout(setUpTabletUI, 100);
    }

    function hideTabletUI() {
        tabletShown = false;
        print("hide tablet-ui");
        if (UIWebTablet) {
            if (UIWebTablet.onClose) {
                UIWebTablet.onClose();
            }

            tabletLocation = UIWebTablet.getLocation();
			UIWebTablet.unregister();
            UIWebTablet.destroy();
            UIWebTablet = null;
            HMD.tabletID = null;
        }
    }

    function updateShowTablet() {
        if (HMD.showTablet && !tabletShown) {
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            hideTabletUI();
        }
    }

    Script.update.connect(updateShowTablet);
    // Script.setInterval(updateShowTablet, 1000);

}()); // END LOCAL_SCOPE
