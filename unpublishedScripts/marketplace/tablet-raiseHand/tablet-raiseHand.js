"use strict";
//
// tablet-raiseHand.js
//
// client script that creates a tablet button to raise hand
//
// Created by Triplelexx on 17/04/22
// Copyright 2017 High Fidelity, Inc.
//
// Hand icons adapted from https://linearicons.com, created by Perxis https://perxis.com CC BY-SA 4.0 license.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var BUTTON_NAME = "RAISE\nHAND";
    var USERCONNECTION_MESSAGE_CHANNEL = "io.highfidelity.makeUserConnection";
    var DEBUG_PREFIX = "TABLET RAISE HAND: ";
    var isRaiseHandButtonActive = false;
    var animHandlerId;

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: BUTTON_NAME,
        icon: "icons/tablet-icons/raise-hand-i.svg",
        activeIcon: "icons/tablet-icons/raise-hand-a.svg"
    });

    function onClicked() {
        isRaiseHandButtonActive = !isRaiseHandButtonActive;
        button.editProperties({ isActive: isRaiseHandButtonActive });
        if (isRaiseHandButtonActive) {
            removeAnimation();
            animHandlerId = MyAvatar.addAnimationStateHandler(raiseHandAnimation, []);
            Messages.subscribe(USERCONNECTION_MESSAGE_CHANNEL);
            Messages.messageReceived.connect(messageHandler);
        } else {
            removeAnimation();
            Messages.unsubscribe(USERCONNECTION_MESSAGE_CHANNEL);
            Messages.messageReceived.disconnect(messageHandler);
        }
    }

    function removeAnimation() {
        if (animHandlerId) {
            animHandlerId = MyAvatar.removeAnimationStateHandler(animHandlerId);
        }
    }

    function raiseHandAnimation(animationProperties) {
        // all we are doing here is moving the right hand to a spot that is above the hips.
        var headIndex = MyAvatar.getJointIndex("Head");
        var offset = 0.0;
        var result = {};
        if (headIndex) {
            offset = 0.85 * MyAvatar.getAbsoluteJointTranslationInObjectFrame(headIndex).y;
        }
        var handPos = Vec3.multiply(offset, { x: -0.7, y: 1.25, z: 0.25 });
        result.rightHandPosition = handPos;
        result.rightHandRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
        return result;
    }

    function messageHandler(channel, messageString, senderID) {
        if (channel !== USERCONNECTION_MESSAGE_CHANNEL && senderID !== MyAvatar.sessionUUID) {
            return;
        }
        var message = {};
        try {
            message = JSON.parse(messageString);
        } catch (e) {
            print(DEBUG_PREFIX + "messageHandler error: " + e);
        }
        switch (message.key) {
            case "waiting":
            case "connecting":
            case "connectionAck":
            case "connectionRequest":
            case "done":
                removeAnimation();
                if (isRaiseHandButtonActive) {
                    isRaiseHandButtonActive = false;
                    button.editProperties({ isActive: isRaiseHandButtonActive });
                }
                break;
            default:
                print(DEBUG_PREFIX + "messageHandler unknown message: " + message);
                break;
        }
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function() {
        Messages.unsubscribe(USERCONNECTION_MESSAGE_CHANNEL);
        Messages.messageReceived.disconnect(messageHandler);
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
        removeAnimation();
    });
}()); // END LOCAL_SCOPE
