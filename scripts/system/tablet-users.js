"use strict";

//
//  tablet-users.js
//
//  Created by Faye Li on 18 Jan 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var USERS_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/faye/tablet-dev/users.html");
    var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

    var FRIENDS_WINDOW_URL = Account.metaverseServerURL + "/user/friends";
    var FRIENDS_WINDOW_WIDTH = 290;
    var FRIENDS_WINDOW_HEIGHT = 500;
    var FRIENDS_WINDOW_TITLE = "Add/Remove Friends";

    // Initialise visibility based on global service
    var VISIBILITY_VALUES_SET = {};
    VISIBILITY_VALUES_SET["all"] = true;
    VISIBILITY_VALUES_SET["friends"] = true;
    VISIBILITY_VALUES_SET["none"] = true;
    var myVisibility;
    if (GlobalServices.findableBy in VISIBILITY_VALUES_SET) {
        myVisibility = GlobalServices.findableBy;
    } else {
        // default to friends if it can't be determined
        myVisibility = "friends";
        GlobalServices.findableBy = myVisibility;
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/users-i.svg",
        activeIcon: "icons/tablet-icons/users-a.svg",
        text: "USERS"
    });

    var onUsersScreen = false;
    var shouldActivateButton = false;

    function onClicked() {
        if (onUsersScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            if (HMD.tabletID) {
                Entities.editEntity(HMD.tabletID, {textures: JSON.stringify({"tex.close" : HOME_BUTTON_TEXTURE})});
            }
            shouldActivateButton = true;
            tablet.gotoWebScreen(USERS_URL);
            onUsersScreen = true;
        }
    }

    function onScreenChanged() {
        // for toolbar mode: change button to active when window is first openend, false otherwise.
        button.editProperties({isActive: shouldActivateButton});
        shouldActivateButton = false;
        onUsersScreen = false;
    }

    function onWebEventReceived(event) {
        if (typeof event === "string") {
            try {
                event = JSON.parse(event);
            } catch(e) {
                return;
            }
        }
        if (event.type === "ready") {
            // send username to html
            var myUsername = GlobalServices.username;
            var object = {
                "type": "user-info",
                "data": {
                    "username": myUsername,
                    "visibility": myVisibility
                }
            };
            tablet.emitScriptEvent(JSON.stringify(object));
        }
        if (event.type === "manage-friends") {
            // open a web overlay to metaverse friends page
            var friendsWindow = new OverlayWebWindow({
                title: FRIENDS_WINDOW_TITLE,
                width: FRIENDS_WINDOW_WIDTH,
                height: FRIENDS_WINDOW_HEIGHT,
                visible: false
            });
            friendsWindow.setURL(FRIENDS_WINDOW_URL);
            friendsWindow.setVisible(true);
            friendsWindow.raise();
        }
        if (event.type === "jump-to") {
            if (typeof event.data.username !== undefined) {
                // teleport to selected user from the online users list
                location.goToUser(event.data.username);
            }
        }
        if (event.type === "toggle-visibility") {
            if (typeof event.data.visibility !== undefined) {
                // update your visibility (all, friends, or none)
                myVisibility = event.data.visibility;
                GlobalServices.findableBy = myVisibility;
            }
        }
    }

    button.clicked.connect(onClicked);
    tablet.webEventReceived.connect(onWebEventReceived);
    tablet.screenChanged.connect(onScreenChanged);

    function cleanup() {
        if (onUsersScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE
