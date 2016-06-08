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

Script.include("libraries/toolBars.js");

function initialPosition(windowDimensions, toolbar) {
    return {
        x: (windowDimensions.x / 2) - (Tool.IMAGE_WIDTH * 1),
        y: windowDimensions.y
    };
}
var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.goto.toolbar", initialPosition, {
    x: -Tool.IMAGE_WIDTH / 2,
    y: -Tool.IMAGE_HEIGHT
});
var button = toolBar.addTool({
    imageURL: Script.resolvePath("assets/images/tools/directory-01.svg"),
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
        DialogsManager.toggleAddressBar();
    }
};
Controller.mousePressEvent.connect(onMousePress)
Script.scriptEnding.connect(function () {
    Controller.mousePressEvent.disconnect(onMousePress)
    toolBar.cleanup();
});
