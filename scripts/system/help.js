"use strict";

//
//  help.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Nov 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        //icon: "help.svg",
        color: "#ff6f6f",
        text: "HELP"
    });

    // TODO: make button state reflect whether the window is opened or closed (independently from us).
    function onClicked(){
        Menu.triggerOption('Help...')
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        tablet.removeButton(buttonName);
        button.clicked.disconnect(onClicked);
    });

}()); // END LOCAL_SCOPE
