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
    icon: "icons/tablet-icons/goto-i.svg",
    text:"GOTO"
});


function onAddressBarShown(visible) {
}

function setActive(active) {
    isActive = active;
}
function onClicked(){
    setActive(!isActive);
    button.editProperties({isActive: isActive});
    DialogsManager.toggleAddressBar();
}
button.clicked.connect(onClicked);
DialogsManager.addressBarShown.connect(onAddressBarShown);

Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    tablet.removeButton(button);
    DialogsManager.addressBarShown.disconnect(onAddressBarShown);
});

}()); // END LOCAL_SCOPE
