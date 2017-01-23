"use strict";

//
//  users.js
//
//  Created by Faye Li on 18 Jan 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var USERS_URL = "https://hifi-content.s3.amazonaws.com/faye/tablet-dev/users.html";
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        // TODO: work with Alan to make new icon art
        icon: "icons/tablet-icons/people-i.svg",
        text: "Users"
    });

    function onClicked() {
        tablet.gotoWebScreen(USERS_URL);
    }

    function onWebEventReceived(event) {
        print("Script received a web event, its type is " + typeof event);
        if (typeof event === "string") {
            event = JSON.parse(event);
        }
        if (event.type === "ready") {
            // send username to html
            var myUsername = GlobalServices.username;
            var object = {
                "type": "sendUsername",
                "data": {"username": myUsername}
            };
            print("sending username: " + myUsername);
            tablet.emitScriptEvent(JSON.stringify(object));
        } 
    }

    button.clicked.connect(onClicked);
    tablet.webEventReceived.connect(onWebEventReceived);

    function cleanup() {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE
