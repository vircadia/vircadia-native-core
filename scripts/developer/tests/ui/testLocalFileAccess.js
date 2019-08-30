"use strict";

//  Created by Bradley Austin Davis on 2019/08/29
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

    
var QML_URL = "qrc:/qml/hifi/tablet/DynamicWebview.qml"
var LOCAL_FILE_URL = "file:///C:/test-file.html"

var TABLET_BUTTON_NAME = "SCRIPT";
var ICONS = {
    icon: "icons/tablet-icons/menu-i.svg",
    activeIcon: "icons/tablet-icons/meni-a.svg"
};


var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: ICONS.icon,
    activeIcon: ICONS.activeIcon,
    text: TABLET_BUTTON_NAME,
    sortOrder: 1
});

function onClicked() {
    var window = Desktop.createWindow(QML_URL, {
        title: "Local file access test",
        additionalFlags: Desktop.ALWAYS_ON_TOP,
        presentationMode: Desktop.PresentationMode.NATIVE,
        size: { x: 640, y: 480 },
        visible: true,
        position: { x: 100, y: 100 },
    });
    window.sendToQml({ url: LOCAL_FILE_URL });
}

button.clicked.connect(onClicked);

Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    tablet.removeButton(button);
});

}()); // END LOCAL_SCOPE



