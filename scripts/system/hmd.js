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

Script.include("libraries/toolBars.js");

var headset; // The preferred headset. Default to the first one found in the following list.
var displayMenuName = "Display";
var desktopMenuItemName = "Desktop";
['OpenVR (Vive)', 'Oculus Rift'].forEach(function (name) {
    if (!headset && Menu.menuItemExists(displayMenuName, name)) {
        headset = name;
    }
});

function initialPosition(windowDimensions, toolbar) {
    return {
        x: windowDimensions.x / 2 + (2 * Tool.IMAGE_WIDTH),
        y: windowDimensions.y
    };
}
var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.hmd.toolbar", initialPosition, {
    x: -Tool.IMAGE_WIDTH / 2,
    y: -Tool.IMAGE_HEIGHT
});
var button = toolBar.addTool({
    imageURL: Script.resolvePath("assets/images/tools/hmd-switch-01.svg"),
    subImage: {
        x: 0,
        y: Tool.IMAGE_WIDTH,
        width: Tool.IMAGE_WIDTH,
        height: Tool.IMAGE_HEIGHT
    },
    width: Tool.IMAGE_WIDTH,
    height: Tool.IMAGE_HEIGHT,
    alpha: 0.9,
    visible: true,
    showButtonDown: true
});
function onMousePress (event) {
    if (event.isLeftButton && button === toolBar.clicked(Overlays.getOverlayAtPoint(event))) {
        var isDesktop = Menu.isOptionChecked(desktopMenuItemName);
        Menu.setIsOptionChecked(isDesktop ? headset : desktopMenuItemName, true);
    }
};
Controller.mousePressEvent.connect(onMousePress)
Script.scriptEnding.connect(function () {
    Controller.mousePressEvent.disconnect(onMousePress)
    toolBar.cleanup();
});
