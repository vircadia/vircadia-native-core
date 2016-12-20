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
var isActive = false
var button = tablet.addButton({
    text:"GOTO"})


function onAddressBarShown(visible) {
}

function setActive(active) {
  isActive = active;
}
function onClicked(){
    setActive(!isActive);
    var buttonProperties = button.getProperties();
    buttonProperties.isActive = isActive;
    button.editProperties(buttonProperties);
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
