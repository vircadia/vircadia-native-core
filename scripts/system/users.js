"use strict";

//
//  users.js
//
//  Created by Faye Li on 18 Jan 2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        // TODO: work with Alan to make new icon art
        icon: "icons/tablet-icons/people-i.svg",
        text: "Users"
    });

    function cleanup() {
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE
