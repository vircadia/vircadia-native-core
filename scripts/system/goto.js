"use strict";

//
//  goto.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var button = tablet.addButton({
    color: "63d0ff",
    text:"GOTO"})
/*var button = toolBar.addButton({
    objectName: "goto",
    imageURL: Script.resolvePath("assets/images/tools/directory.svg"),
    visible: true,
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9,
});*/

function onAddressBarShown(visible) {
    //button.writeProperty('buttonState', visible ? 0 : 1);
    //button.writeProperty('defaultState', visible ? 0 : 1);
    //button.writeProperty('hoverState', visible ? 2 : 3);
}
function onClicked(){
    DialogsManager.toggleAddressBar();
}
button.clicked.connect(onClicked);
DialogsManager.addressBarShown.connect(onAddressBarShown);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("goto");
    button.clicked.disconnect(onClicked);
    DialogsManager.addressBarShown.disconnect(onAddressBarShown);
});

}()); // END LOCAL_SCOPE
