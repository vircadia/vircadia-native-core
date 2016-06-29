//
//  hmd.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var headset; // The preferred headset. Default to the first one found in the following list.
var displayMenuName = "Display";
var desktopMenuItemName = "Desktop";
['OpenVR (Vive)', 'Oculus Rift'].forEach(function (name) {
    if (!headset && Menu.menuItemExists(displayMenuName, name)) {
        headset = name;
    }
});

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var button;

if (headset) {
    button = toolBar.addButton({
        objectName: "hmdToggle",
        imageURL: Script.resolvePath("assets/images/tools/hmd-switch-01.svg"),
        visible: true,
        yOffset: 50,
        alpha: 0.9,
    });
    
    button.clicked.connect(function(){
        var isDesktop = Menu.isOptionChecked(desktopMenuItemName);
        Menu.setIsOptionChecked(isDesktop ? headset : desktopMenuItemName, true);
    });
    
    Script.scriptEnding.connect(function () {
        button.clicked.disconnect();
    });
}

