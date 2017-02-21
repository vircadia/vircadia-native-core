"use strict";

//
//  goto.js
//  scripts/system/
//
//  Created by Dante Ruiz on 8 February 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var gotoQmlSource = "TabletAddressDialog.qml";
    var buttonName = "GOTO";

    function onClicked(){
        tablet.loadQMLSource(gotoQmlSource);
    }
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/goto-i.svg",
        activeIcon: "icons/tablet-icons/goto-a.svg",
        text: buttonName,
        sortOrder: 8
    });

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        if (tablet) {
            tablet.removeButton(button);
        }
    });

}()); // END LOCAL_SCOPE
