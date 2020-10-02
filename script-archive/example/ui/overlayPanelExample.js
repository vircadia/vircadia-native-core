//
//  overlayPanelExample.js
//  examples/example/ui
//
//  Created by Alexander Otavka
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    "../../libraries/globals.js",
    "../../libraries/overlayManager.js",
]);

var BG_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/card-bg.svg";
var RED_DOT_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/red-dot.svg";
var BLUE_SQUARE_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/blue-square.svg";

var mainPanel = new OverlayPanel({
    anchorPositionBinding: { avatar: "MyAvatar" },
    offsetPosition: { x: 0, y: 0.4, z: -1 },
    isFacingAvatar: false
});

var bluePanel = mainPanel.addChild(new OverlayPanel ({
    offsetPosition: { x: 0.1, y: 0.1, z: 0.2 },
    offsetScale: 0.5
}));

var mainPanelBackground = new Image3DOverlay({
    url: BG_IMAGE_URL,
    dimensions: {
        x: 0.5,
        y: 0.5,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0,
        y: 0,
        z: -0.001
    }
});

var bluePanelBackground = mainPanelBackground.clone();

mainPanel.addChild(mainPanelBackground);
bluePanel.addChild(bluePanelBackground);

var textWidth = .25;
var textHeight = .1;
var numberOfLines = 1;
var textMargin = 0.00625;
var lineHeight = (textHeight - (2 * textMargin)) / numberOfLines;

var text = mainPanel.addChild(new Text3DOverlay({
    text: "TEXT",
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.1,
        y: -0.15,
        z: 0.001
    },
    dimensions: { x: textWidth, y: textHeight },
    backgroundColor: { red: 0, green: 0, blue: 0 },
    color: { red: 255, green: 255, blue: 255 },
    topMargin: textMargin,
    leftMargin: textMargin,
    bottomMargin: textMargin,
    rightMargin: textMargin,
    lineHeight: lineHeight,
    alpha: 0.9,
    backgroundAlpha: 0.9
}));

var redDot = mainPanel.addChild(new Image3DOverlay({
    url: RED_DOT_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.15,
        y: -0.15,
        z: 0
    }
}));

var redDot2 = mainPanel.addChild(new Image3DOverlay({
    url: RED_DOT_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.155,
        y: 0.005,
        z: 0
    }
}));

var blueSquare = bluePanel.addChild(new Image3DOverlay({
    url: BLUE_SQUARE_IMAGE_URL,
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.09,
        y: -0.09,
        z: 0
    }
}));

var blueSquare2 = bluePanel.addChild(new Image3DOverlay({
    url: BLUE_SQUARE_IMAGE_URL,
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.09,
        y: 0.09,
        z: 0
    }
}));

var blueSquare3 = blueSquare2.clone();
blueSquare3.offsetPosition = {
    x: -0.09,
    y: 0.09,
    z: 0
};

var mouseDown = {};

function onMouseDown(event) {
    if (event.isLeftButton) {
        mouseDown.overlay = OverlayManager.findAtPoint({ x: event.x, y: event.y });
    }
    if (event.isRightButton) {
        mouseDown.pos = { x: event.x, y: event.y };
    }
    mouseDown.maxDistance = 0;
}

function onMouseMove(event) {
    if (mouseDown.maxDistance !== undefined) {
        var dist = Vec3.distance(mouseDown.pos, { x: event.x, y: event.y });
        if (dist > mouseDown.maxDistance) {
            mouseDown.maxDistance = dist;
        }
    }
}

function onMouseUp(event) {
    if (event.isLeftButton) {
        var overlay = OverlayManager.findAtPoint({ x: event.x, y: event.y });
        if (overlay && overlay === mouseDown.overlay) {
            if (overlay.parentPanel === bluePanel) {
                overlay.destroy();
            } else {
                overlay.offsetPosition = Vec3.sum(overlay.offsetPosition, { x: 0, y: 0, z: -0.1 });
            }
        }
    }
    if (event.isRightButton && mouseDown.maxDistance < 10) {
        mainPanel.visible = !mainPanel.visible;
    }
}

function onScriptEnd() {
    mainPanel.destroy();
}

Controller.mousePressEvent.connect(onMouseDown);
Controller.mouseMoveEvent.connect(onMouseMove);
Controller.mouseReleaseEvent.connect(onMouseUp);
Script.scriptEnding.connect(onScriptEnd);
