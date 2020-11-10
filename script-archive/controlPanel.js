//
//  controlPanel.js
//  examples
//
//  Created by Zander Otavka on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Shows a few common controls in a OverlayPanel on right click.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    "libraries/globals.js",
    "libraries/overlayManager.js",
]);

var BG_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/card-bg.svg";
var CLOSE_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/tools/close.svg";
var MIC_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/tools/mic-toggle.svg";
var FACE_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/tools/face-toggle.svg";
var ADDRESS_BAR_IMAGE_URL = VIRCADIA_PUBLIC_CDN + "images/tools/address-bar-toggle.svg";

var panel = new OverlayPanel({
    anchorPositionBinding: { avatar: "MyAvatar" },
    offsetPosition: { x: 0, y: 0.4, z: -1 },
    visible: false
});

var background = new Image3DOverlay({
    url: BG_IMAGE_URL,
    dimensions: {
        x: 0.5,
        y: 0.5,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    visible: false
});
panel.addChild(background);

var closeButton = new Image3DOverlay({
    url: CLOSE_IMAGE_URL,
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.1,
        y: 0.1,
        z: 0.001
    },
    visible: false
});
closeButton.onClick = function(event) {
    panel.visible = false;
};
panel.addChild(closeButton);

var micMuteButton = new Image3DOverlay({
    url: MIC_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.1,
        y: 0.1,
        z: 0.001
    },
    visible: false
});
micMuteButton.onClick = function(event) {
    AudioDevice.toggleMute();
};
panel.addChild(micMuteButton);

var faceMuteButton = new Image3DOverlay({
    url: FACE_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.1,
        y: -0.1,
        z: 0.001
    },
    visible: false
});
faceMuteButton.onClick = function(event) {
    FaceTracker.toggleMute();
};
panel.addChild(faceMuteButton);

var addressBarButton = new Image3DOverlay({
    url: ADDRESS_BAR_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.1,
        y: -0.1,
        z: 0.001
    },
    visible: false
});
addressBarButton.onClick = function(event) {
    DialogsManager.toggleAddressBar();
};
panel.addChild(addressBarButton);

panel.setChildrenVisible();


function onMicMuteToggled() {
    var offset;
    if (AudioDevice.getMuted()) {
        offset = 45;
    } else {
        offset = 0;
    }
    micMuteButton.subImage = {
        x: offset,
        y: 0,
        width: 45,
        height: 45
    };
}
onMicMuteToggled();

function onFaceMuteToggled() {
    var offset;
    if (FaceTracker.getMuted()) {
        offset = 45;
    } else {
        offset = 0;
    }
    faceMuteButton.subImage = {
        x: offset,
        y: 0,
        width: 45,
        height: 45
    };
}
onFaceMuteToggled();

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
        if (overlay && overlay === mouseDown.overlay && overlay.onClick) {
            overlay.onClick(event);
        }
    }
    if (event.isRightButton && mouseDown.maxDistance < 10) {
        panel.setProperties({
            visible: !panel.visible,
            anchorRotation: MyAvatar.orientation
        });
    }

    mouseDown = {};
}

function onScriptEnd(event) {
    panel.destroy();
}

Controller.mousePressEvent.connect(onMouseDown);
Controller.mouseMoveEvent.connect(onMouseMove);
Controller.mouseReleaseEvent.connect(onMouseUp);
AudioDevice.muteToggled.connect(onMicMuteToggled);
FaceTracker.muteToggled.connect(onFaceMuteToggled);
Script.scriptEnding.connect(onScriptEnd);
