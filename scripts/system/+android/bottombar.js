"use strict";
//
//  bottombar.js
//  scripts/system/
//
//  Created by Gabriel Calero & Cristian Duarte on Jan 18, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() { // BEGIN LOCAL_SCOPE

var bottombar;
var bottomHudOptionsBar;
var gotoBtn;

var gotoScript = Script.require('./goto.js');

var logEnabled = false;

function printd(str) {
    if (logEnabled) {    	
        print("[bottombar.js] " + str);
    }
}

function init() {
    gotoScript.init();
    gotoScript.setOnShownChange(function (shown) {
        if (shown) {
            showAddressBar();
        } else {
            hideAddressBar();
        }
    });

	setupBottomBar();
	setupBottomHudOptionsBar();

    raiseBottomBar();

}

function shutdown() {
}

function setupBottomBar() {
    bottombar = new QmlFragment({
        qml: "hifi/bottombar.qml"
    });

    bottombar.fromQml.connect(function(message) {
        switch (message.method) {
        case 'hide':
            lowerBottomBar();
            break;
        default:
            print('[bottombar.js] Unrecognized message from bottomHud.qml:', JSON.stringify(message));
        }
    });


    gotoBtn = bottombar.addButton({
        icon: "icons/goto-i.svg",
        activeIcon: "icons/goto-a.svg",
        bgOpacity: 0,
        hoverBgOpacity: 0,
        activeBgOpacity: 0,
        activeHoverBgOpacity: 0,
        height: 240,
        width: 300,
        iconSize: 108,
        textSize: 45,
        text: "GO TO"
    });

    gotoBtn.clicked.connect(function() {
        if (!gotoScript.isVisible()) {
            showAddressBar();
        } else {
            hideAddressBar();
        }
    });

    // TODO: setup all the buttons or provide a dynamic interface

	raiseBottomBar();


}

var setupBottomHudOptionsBar = function() {
    var bottomHud = new QmlFragment({
        qml: "hifi/bottomHudOptions.qml"
    });

    bottomHudOptionsBar = {
        show: function() {
            bottomHud.setVisible(true);
        },
        hide: function() {
            bottomHud.setVisible(false);
        },
        qmlFragment: bottomHud
    };
    bottomHud.fromQml.connect(
        function(message) {
            switch (message.method) {
                case 'showUpBar':
                    printd('[bottombar.js] showUpBar message from bottomHudOptions.qml: ', JSON.stringify(message));
                    raiseBottomBar();
                    break;
                default:
                    print('[bottombar.js] Unrecognized message from bottomHudOptions.qml:', JSON.stringify(message));
            }
        }
    );
}

function lowerBottomBar() {
    if (bottombar) {
        bottombar.setVisible(false);
    }
    if (bottomHudOptionsBar) {
        bottomHudOptionsBar.show();
    }
}

function raiseBottomBar() {
    print('[bottombar.js] raiseBottomBar begin');
    if (bottombar) {
        bottombar.setVisible(true);
    }
    if (bottomHudOptionsBar) {
        bottomHudOptionsBar.hide();
    }
    print('[bottombar.js] raiseBottomBar end');
}

function showAddressBar() {
    gotoScript.show();
    gotoBtn.isActive = true;
}

function hideAddressBar() {
    gotoScript.hide();
    gotoBtn.isActive = false;
}



Script.scriptEnding.connect(function () {
	shutdown();
});

init();

}()); // END LOCAL_SCOPE
