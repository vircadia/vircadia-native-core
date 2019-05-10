//
//  SimplifiedTopBar.qml
//
//  Created by Zach Fox on 2019-05-02
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../simplifiedConstants" as SimplifiedConstants
import "../inputDeviceButton" as InputDeviceButton
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import QtGraphicalEffects 1.0

Rectangle {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent;


    Item {
        id: avatarButtonContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        width: height

        Image {
            id: avatarButtonImage
            source: "images/defaultAvatar.jpg"
            anchors.centerIn: parent
            width: parent.width - 10
            height: width
            mipmap: true
            fillMode: Image.PreserveAspectCrop
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: mask
            }

            MouseArea {
                id: avatarButtonImageMouseArea
                anchors.fill: parent
                hoverEnabled: enabled
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    
                    if (Account.loggedIn) {
                        sendToScript({
                            "source": "SimplifiedTopBar.qml",
                            "method": "toggleAvatarApp"
                        });
                    } else {
                        DialogsManager.showLoginDialog();
                    }
                }
            }
        }

        Rectangle {
            z: -1
            id: borderMask
            visible: avatarButtonImageMouseArea.containsMouse
            width: avatarButtonImage.width + 4
            height: width
            radius: width
            anchors.centerIn: avatarButtonImage
            color: "#FFFFFF"
        }

        Rectangle {
            id: mask
            anchors.fill: avatarButtonImage
            radius: avatarButtonImage.width
            visible: false
        }
    }


    InputDeviceButton.InputDeviceButton {
        id: inputDeviceButton
        anchors.left: avatarButtonContainer.right
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }


    Item {
        id: outputDeviceButtonContainer
        anchors.left: inputDeviceButton.right
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 40

        HifiStylesUit.HiFiGlyphs {
            property bool outputMuted: false
            id: outputDeviceButton
            text: (outputDeviceButton.outputMuted ? simplifiedUI.glyphs.vol_0 : simplifiedUI.glyphs.vol_3)
            color: (outputDeviceButton.outputMuted ? simplifiedUI.colors.controls.outputVolumeButton.text.muted : simplifiedUI.colors.controls.outputVolumeButton.text.noisy)
            opacity: outputDeviceButtonMouseArea.containsMouse ? 1.0 : 0.7
            size: 40
            anchors.centerIn: parent
            width: 35
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: outputDeviceButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    outputDeviceButton.outputMuted = !outputDeviceButton.outputMuted;

                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "setOutputMuted",
                        "data": {
                            "outputMuted": outputDeviceButton.outputMuted
                        }
                    });
                }
            }
        }
    }



    Item {
        id: hmdButtonContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: settingsButtonContainer.left
        anchors.rightMargin: 8
        width: height

        HifiStylesUit.HiFiGlyphs {
            id: hmdGlyph
            text: HMD.active ? simplifiedUI.glyphs.hmd : simplifiedUI.glyphs.screen
            color: simplifiedUI.colors.text.white
            opacity: hmdGlyphMouseArea.containsMouse ? 1.0 : 0.7
            size: 40
            anchors.centerIn: parent
            width: 35
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: hmdGlyphMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    // TODO: actually do this right and change the display plugin
                    HMD.active = !HMD.active;
                }
            }
        }
    }



    Item {
        id: settingsButtonContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        width: height

        HifiStylesUit.HiFiGlyphs {
            id: settingsGlyph
            text: simplifiedUI.glyphs.gear
            color: simplifiedUI.colors.text.white
            opacity: settingsGlyphMouseArea.containsMouse ? 1.0 : 0.7
            size: 40
            anchors.centerIn: parent
            width: 35
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: settingsGlyphMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "SimplifiedTopBar.qml",
                        "method": "toggleSettingsApp"
                    });
                }
            }
        }
    }


    function fromScript(message) {
        if (message.source !== "simplifiedUI.js") {
            return;
        }

        switch (message.method) {
            case "updateAvatarThumbnailURL":
                avatarButtonImage.source = message.data.avatarThumbnailURL;
                break;

            case "updateOutputMuted":
                outputDeviceButton.outputMuted = message.data.outputMuted;
                break;

            default:
                console.log('SimplifiedTopBar.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
