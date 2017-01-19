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
    var USERS_URL = Script.resolvePath("html/users.html");
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        // TODO: work with Alan to make new icon art
        icon: "icons/tablet-icons/people-i.svg",
        text: "Users"
    });

    function onClicked() {
        tablet.gotoWebScreen(USERS_URL);
    }

    button.clicked.connect(onClicked);

    function cleanup() {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE
