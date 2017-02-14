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
    var button;
    var buttonName = "GOTO";
    var toolBar = null;
    var tablet = null;
    function onAddressBarShown(visible) {
        if (toolBar) { 
            button.editProperties({isActive: visible});
        }
    }

    function onClicked(){
        if (toolBar) {
            DialogsManager.toggleAddressBar();
        } else {
            tablet.loadQMLSource(gotoQmlSource);
        }
    }
    if (Settings.getValue("HUDUIEnabled")) {
        toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
        button = toolBar.addButton({
            objectName: buttonName,
            imageURL: Script.resolvePath("assets/images/tools/directory.svg"),
            visible: true,
            alpha: 0.9
        });
    } else {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        button = tablet.addButton({
            icon: "icons/tablet-icons/goto-i.svg",
            activeIcon: "icons/tablet-icons/goto-a.svg",
            text: buttonName,
            sortOrder: 8
        });
    }
    
    button.clicked.connect(onClicked);
    DialogsManager.addressBarShown.connect(onAddressBarShown);
    
    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        if (tablet) {
            tablet.removeButton(button);
        }
        if (toolBar) {
            toolBar.removeButton(buttonName);
        }
        DialogsManager.addressBarShown.disconnect(onAddressBarShown);
    });
    
}()); // END LOCAL_SCOPE
