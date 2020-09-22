//
//  avatarAttachmentTest.js
//  examples/tests
//
//  Created by Anthony Thibault on January 7, 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Test for MyAvatar attachment API
// MyAvatar.setAttachmentData();
// MyAvatar.getAttachmentData();

// Toggle button helper
function ToggleButtonBuddy(x, y, width, height, urls) {
    this.up = Overlays.addOverlay("image", {
        x: x, y: y, width: width, height: height,
        subImage: { x: 0, y: 0, width: width, height: height},
        imageURL: urls.up,
        visible: true,
        alpha: 1.0
    });
    this.down = Overlays.addOverlay("image", {
        x: x, y: y, width: width, height: height,
        subImage: { x: 0, y: 0, width: width, height: height},
        imageURL: urls.down,
        visible: false,
        alpha: 1.0
    });
    this.callbacks = [];

    var self = this;
    Controller.mousePressEvent.connect(function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
        if (clickedOverlay === self.up) {
            // flip visiblity
            Overlays.editOverlay(self.up, {visible: false});
            Overlays.editOverlay(self.down, {visible: true});
            self.onToggle(true);
        } else if (clickedOverlay === self.down) {
            // flip visiblity
            Overlays.editOverlay(self.up, {visible: true});
            Overlays.editOverlay(self.down, {visible: false});
            self.onToggle(false);
        }
    });
}
ToggleButtonBuddy.prototype.destroy = function () {
    Overlays.deleteOverlay(this.up);
    Overlays.deleteOverlay(this.down);
};
ToggleButtonBuddy.prototype.addToggleHandler = function (callback) {
    this.callbacks.push(callback);
    return callback;
};
ToggleButtonBuddy.prototype.removeToggleHandler = function (callback) {
    var index = this.callbacks.indexOf(callback);
    if (index !== -1) {
        this.callbacks.splice(index, 1);
    }
};
ToggleButtonBuddy.prototype.onToggle = function (isDown) {
    var i, l = this.callbacks.length;
    for (i = 0; i < l; i++) {
        this.callbacks[i](isDown);
    }
};

var windowDimensions = Controller.getViewportDimensions();
var BUTTON_WIDTH = 64;
var BUTTON_HEIGHT = 64;
var BUTTON_PADDING = 10;
var buttonPositionX = windowDimensions.x - BUTTON_PADDING - BUTTON_WIDTH;
var buttonPositionY = (windowDimensions.y - BUTTON_HEIGHT) / 2 - (BUTTON_HEIGHT + BUTTON_PADDING);

var hatButton = new ToggleButtonBuddy(buttonPositionX, buttonPositionY, BUTTON_WIDTH, BUTTON_HEIGHT, {
    up: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/hat-up.svg"),
    down: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/hat-down.svg")
});

buttonPositionY += BUTTON_HEIGHT + BUTTON_PADDING;
var coatButton = new ToggleButtonBuddy(buttonPositionX, buttonPositionY, BUTTON_WIDTH, BUTTON_HEIGHT, {
    up: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/coat-up.svg"),
    down: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/coat-down.svg")
});

buttonPositionY += BUTTON_HEIGHT + BUTTON_PADDING;
var coatButton2 = new ToggleButtonBuddy(buttonPositionX, buttonPositionY, BUTTON_WIDTH, BUTTON_HEIGHT, {
    up: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/coat-up.svg"),
    down: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/coat-down.svg")
});

var AVATAR_ATTACHMENT = 0;
var AVATAR_SOFT_ATTACHMENT = 1;
var ENTITY_ATTACHMENT = 2;

var HAT_ATTACHMENT = {
    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/cowboy-hat.fbx"),
    jointName: "Head",
    translation: {"x": 0, "y": 0.25, "z": 0.03},
    rotation: {"x": 0, "y": 0, "z": 0, "w": 1},
    scale: 0.052,
    type: AVATAR_ATTACHMENT
};

var COAT_ATTACHMENT = {
    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ozan/dev/clothes/coat/coat_rig.fbx"),
    jointName: "Hips",
    translation: {"x": 0, "y": 0, "z": 0},
    rotation: {"x": 0, "y": 0, "z": 0, "w": 1},
    scale: 1,
    type: AVATAR_SOFT_ATTACHMENT
};

var COAT_ENTITY_ATTACHMENT = {
    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ozan/dev/clothes/coat/coat_rig.fbx"),
    jointName: "Hips",
    translation: {"x": 0, "y": 0, "z": 0},
    rotation: {"x": 0, "y": 0, "z": 0, "w": 1},
    scale: 1,
    type: ENTITY_ATTACHMENT
};

hatButton.addToggleHandler(function (isDown) {
    if (isDown) {
        wearAttachment(HAT_ATTACHMENT);
    } else {
        removeAttachment(HAT_ATTACHMENT);
    }
});

coatButton.addToggleHandler(function (isDown) {
    if (isDown) {
        wearAttachment(COAT_ATTACHMENT);
    } else {
        removeAttachment(COAT_ATTACHMENT);
    }
});

coatButton2.addToggleHandler(function (isDown) {
    if (isDown) {
        wearAttachment(COAT_ENTITY_ATTACHMENT);
    } else {
        removeAttachment(COAT_ENTITY_ATTACHMENT);
    }
});

function wearAttachment(attachment) {
    if (attachment.type === AVATAR_ATTACHMENT || attachment.type === AVATAR_SOFT_ATTACHMENT) {
        MyAvatar.attach(attachment.modelURL,
                        attachment.jointName,
                        attachment.translation,
                        attachment.rotation,
                        attachment.scale,
                        (attachment.type === AVATAR_SOFT_ATTACHMENT));
    } else {
        attachment.entityID = Entities.addEntity({
            name: "attachment",
            type: "Model",
            modelURL: attachment.modelURL,
            parentID: MyAvatar.sessionUUID,
            relayParentJoints: true,
            position: attachment.position,
            rotation: attachment.rotation,
            parentJointIndex: -1
        }, true);
    }
}

function removeAttachment(attachment) {
    if (attachment.type === AVATAR_ATTACHMENT || attachment.type === AVATAR_SOFT_ATTACHMENT) {
        var attachments = MyAvatar.attachmentData;
        var i, l = attachments.length;
        for (i = 0; i < l; i++) {
            if (attachments[i].modelURL === attachment.modelURL) {
                attachments.splice(i, 1);
                MyAvatar.attachmentData = attachments;
                break;
            }
        }
    } else {
        Entities.deleteEntity(attachment.entityID);
    }
}

Script.scriptEnding.connect(function() {
    hatButton.destroy();
    coatButton.destroy();
    coatButton2.destroy();
});
