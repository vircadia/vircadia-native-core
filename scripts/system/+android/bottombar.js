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
var avatarBtn;
var bubbleBtn;
var loginBtn;

var gotoScript = Script.require('./goto.js');
var avatarSelection = Script.require('./avatarSelection.js');

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
    avatarSelection.init();
    App.fullAvatarURLChanged.connect(processedNewAvatar);

	setupBottomBar();
	setupBottomHudOptionsBar();

    raiseBottomBar();

    GlobalServices.connected.connect(handleLogin);
    GlobalServices.disconnected.connect(handleLogout);
}

function shutdown() {
    App.fullAvatarURLChanged.disconnect(processedNewAvatar);
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

    avatarBtn = bottombar.addButton({
        icon: "icons/avatar-i.svg",
        activeIcon: "icons/avatar-a.svg",
        bgOpacity: 0,
        height: 240,
        width: 294,
        hoverBgOpacity: 0,
        activeBgOpacity: 0,
        activeHoverBgOpacity: 0,
        iconSize: 108,
        textSize: 45,
        text: "AVATAR"
    });
    avatarBtn.clicked.connect(function() {
        printd("Avatar button clicked");
        if (!avatarSelection.isVisible()) {
            showAvatarSelection();
        } else {
            hideAvatarSelection();
        }
    });
    avatarSelection.onHidden = function() {
        if (avatarBtn) {
            avatarBtn.isActive = false;
        }
    };

    gotoBtn = bottombar.addButton({
        icon: "icons/goto-i.svg",
        activeIcon: "icons/goto-a.svg",
        bgOpacity: 0,
        hoverBgOpacity: 0,
        activeBgOpacity: 0,
        activeHoverBgOpacity: 0,
        height: 240,
        width: 294,
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

    bubbleBtn = bottombar.addButton({
        icon: "icons/bubble-i.svg",
        activeIcon: "icons/bubble-a.svg",
        bgOpacity: 0,
        hoverBgOpacity: 0,
        activeBgOpacity: 0,
        activeHoverBgOpacity: 0,
        height: 240,
        width: 294,
        iconSize: 108,
        textSize: 45,
        text: "BUBBLE"
    });

    bubbleBtn.editProperties({isActive: Users.getIgnoreRadiusEnabled()});

    bubbleBtn.clicked.connect(function() {
        Users.toggleIgnoreRadius();
        bubbleBtn.editProperties({isActive: Users.getIgnoreRadiusEnabled()});
    });

    loginBtn = bottombar.addButton({
        icon: "icons/login-i.svg",
        activeIcon: "icons/login-a.svg",
        height: 240,
        width: 294,
        iconSize: 108,
        textSize: 45,
        text: Account.isLoggedIn() ? "LOG OUT" : "LOG IN"
    });
    loginBtn.clicked.connect(function() {
        if (!Account.isLoggedIn()) {
            Account.checkAndSignalForAccessToken();
        } else {
            Menu.triggerOption("Login / Sign Up");
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
    Controller.setVPadExtraBottomMargin(0);
}

function raiseBottomBar() {
    print('[bottombar.js] raiseBottomBar begin');
    if (bottombar) {
        bottombar.setVisible(true);
    }
    if (bottomHudOptionsBar) {
        bottomHudOptionsBar.hide();
    }
    Controller.setVPadExtraBottomMargin(255); // Height in bottombar.qml
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

function showAvatarSelection() {
    avatarSelection.show();
    avatarBtn.isActive = true;
}

function hideAvatarSelection() {
    avatarSelection.hide();
    avatarBtn.isActive = false;
}

// TODO: Move to avatarSelection.js and make it possible to hide the window from there AND switch the button state here too
function processedNewAvatar(url, modelName) {
    avatarSelection.refreshSelectedAvatar(url);
    hideAvatarSelection();
}

function handleLogin() {
    Script.setTimeout(function() {
        if (Account.isLoggedIn()) {
            MyAvatar.displayName=Account.getUsername();
        }
    }, 2000);
    if (loginBtn) {
        loginBtn.editProperties({text: "LOG OUT"});
    }
}
function handleLogout() {
    MyAvatar.displayName="";
    if (loginBtn) {
        loginBtn.editProperties({text: "LOG IN"});
    }
}

Script.scriptEnding.connect(function () {
	shutdown();
    GlobalServices.connected.disconnect(handleLogin);
    GlobalServices.disconnected.disconnect(handleLogout);
});

init();

}()); // END LOCAL_SCOPE
