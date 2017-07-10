//
//  SpectatorCamera.qml
//  qml/hifi
//
//  Spectator Camera
//
//  Created by Zach Fox on 2017-06-05
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"
import "../controls-uit" as HifiControlsUit
import "../controls" as HifiControls

// references HMD, XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: spectatorCamera;
    // Style
    color: hifi.colors.baseGray;

    // The letterbox used for popup messages
    LetterboxMessage {
        id: letterboxMessage;
        z: 999; // Force the popup on top of everything else
    }
    function letterbox(headerGlyph, headerText, message) {
        letterboxMessage.headerGlyph = headerGlyph;
        letterboxMessage.headerText = headerText;
        letterboxMessage.text = message;
        letterboxMessage.visible = true;
        letterboxMessage.popupRadius = 0;
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: spectatorCamera.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // "Spectator" text
        RalewaySemiBold {
            id: titleBarText;
            text: "Spectator";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.fill: parent;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // TITLE BAR END
    //

    //
    // SPECTATOR APP DESCRIPTION START
    //
    Item {
        id: spectatorDescriptionContainer;
        // Size
        width: spectatorCamera.width;
        height: childrenRect.height;
        // Anchors
        anchors.left: parent.left;
        anchors.top: titleBarContainer.bottom;

        // (i) Glyph
        HiFiGlyphs {
            id: spectatorDescriptionGlyph;
            text: hifi.glyphs.info;
            // Size
            width: 20;
            height: parent.height;
            size: 60;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.top: parent.top;
            anchors.topMargin: 0;
            // Style
            color: hifi.colors.lightGrayText;
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignTop;
        }

        // "Spectator" app description text
        RalewayLight {
            id: spectatorDescriptionText;
            text: "Spectator lets you change what your monitor displays while you're using a VR headset. Use Spectator when streaming and recording video.";
            // Text size
            size: 14;
            // Size
            width: 350;
            height: paintedHeight;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 15;
            anchors.left: spectatorDescriptionGlyph.right;
            anchors.leftMargin: 40;
            // Style
            color: hifi.colors.lightGrayText;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // "Learn More" text
        RalewayRegular {
            id: spectatorLearnMoreText;
            text: "Learn More About Spectator";
            // Text size
            size: 14;
            // Size
            width: paintedWidth;
            height: paintedHeight;
            // Anchors
            anchors.top: spectatorDescriptionText.bottom;
            anchors.topMargin: 10;
            anchors.left: spectatorDescriptionText.anchors.left;
            anchors.leftMargin: spectatorDescriptionText.anchors.leftMargin;
            // Style
            color: hifi.colors.blueAccent;
            wrapMode: Text.WordWrap;
            font.underline: true;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: enabled;
                onClicked: {
                    letterbox(hifi.glyphs.question,
                        "Spectator Camera",
                        "By default, your monitor shows a preview of what you're seeing in VR. " +
                        "Using the Spectator Camera app, your monitor can display the view " +
                        "from a virtual hand-held camera - perfect for taking selfies or filming " +
                        "your friends!<br>" +
                        "<h3>Streaming and Recording</h3>" +
                        "We recommend OBS for streaming and recording the contents of your monitor to services like " +
                        "Twitch, YouTube Live, and Facebook Live.<br><br>" +
                        "To get started using OBS, click this link now. The page will open in an external browser:<br>" +
                        '<font size="4"><a href="https://obsproject.com/forum/threads/official-overview-guide.402/">OBS Official Overview Guide</a></font>');
                }
                onEntered: parent.color = hifi.colors.blueHighlight;
                onExited: parent.color = hifi.colors.blueAccent;
            }
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: spectatorLearnMoreText.bottom;
            anchors.topMargin: spectatorDescriptionText.anchors.topMargin;
        }
    }
    //
    // SPECTATOR APP DESCRIPTION END
    //


    //
    // SPECTATOR CONTROLS START
    //
    Item {
        id: spectatorControlsContainer;
        // Size
        height: spectatorCamera.height - spectatorDescriptionContainer.height - titleBarContainer.height;
        // Anchors
        anchors.top: spectatorDescriptionContainer.bottom;
        anchors.topMargin: 20;
        anchors.left: parent.left;
        anchors.leftMargin: 25;
        anchors.right: parent.right;
        anchors.rightMargin: anchors.leftMargin;

        // "Camera On" Checkbox
        HifiControlsUit.CheckBox {
            id: cameraToggleCheckBox;
            colorScheme: hifi.colorSchemes.dark;
            anchors.left: parent.left;
            anchors.top: parent.top;
            text: "Spectator Camera On";
            boxSize: 24;
            onClicked: {
                sendToScript({method: (checked ? 'spectatorCameraOn' : 'spectatorCameraOff')});
                spectatorCameraPreview.ready = checked;
            }
        }

        // Instructions or Preview
        Rectangle {
            id: spectatorCameraImageContainer;
            anchors.left: parent.left;
            anchors.top: cameraToggleCheckBox.bottom;
            anchors.topMargin: 20;
            anchors.right: parent.right;
            height: 250;
            color: cameraToggleCheckBox.checked ? "transparent" : "black";

            AnimatedImage {
                source: "../../images/static.gif"
                visible: !cameraToggleCheckBox.checked;
                anchors.fill: parent;
                opacity: 0.15;
            }

            // Instructions (visible when display texture isn't set)
            FiraSansRegular {
                id: spectatorCameraInstructions;
                text: "Turn on Spectator Camera for a preview\nof what your monitor shows.";
                size: 16;
                color: hifi.colors.lightGrayText;
                visible: !cameraToggleCheckBox.checked;
                anchors.fill: parent;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }

            // Spectator Camera Preview
            Hifi.ResourceImageItem {
                id: spectatorCameraPreview;
                visible: cameraToggleCheckbox.checked;
                url: monitorShowsSwitch.checked ? "resource://spectatorCameraFrame" : "resource://hmdPreviewFrame";
                ready: cameraToggleCheckBox.checked;
                mirrorVertically: true;
                anchors.fill: parent;
                onVisibleChanged: {
                    ready = cameraToggleCheckBox.checked;
                    update();
                }
            }
        }


        // "Monitor Shows" Switch Label Glyph
        HiFiGlyphs {
            id: monitorShowsSwitchLabelGlyph;
            text: hifi.glyphs.screen;
            size: 32;
            color: hifi.colors.blueHighlight;
            anchors.top: spectatorCameraImageContainer.bottom;
            anchors.topMargin: 13;
            anchors.left: parent.left;
        }
        // "Monitor Shows" Switch Label
        RalewayLight {
            id: monitorShowsSwitchLabel;
            text: "MONITOR SHOWS:";
            anchors.top: spectatorCameraImageContainer.bottom;
            anchors.topMargin: 20;
            anchors.left: monitorShowsSwitchLabelGlyph.right;
            anchors.leftMargin: 6;
            size: 16;
            width: paintedWidth;
            height: paintedHeight;
            color: hifi.colors.lightGrayText;
            verticalAlignment: Text.AlignVCenter;
        }
        // "Monitor Shows" Switch
        HifiControlsUit.Switch {
            id: monitorShowsSwitch;
            height: 30;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: monitorShowsSwitchLabel.bottom;
            anchors.topMargin: 10;
            labelTextOff: "HMD Preview";
            labelTextOn: "Camera View";
            labelGlyphOnText: hifi.glyphs.alert;
            onCheckedChanged: {
                sendToScript({method: 'setMonitorShowsCameraView', params: checked});
            }
        }

        // "Switch View From Controller" Checkbox
        HifiControlsUit.CheckBox {
            id: switchViewFromControllerCheckBox;
            colorScheme: hifi.colorSchemes.dark;
            anchors.left: parent.left;
            anchors.top: monitorShowsSwitch.bottom;
            anchors.topMargin: 25;
            text: "";
            boxSize: 24;
            onClicked: {
                sendToScript({method: 'changeSwitchViewFromControllerPreference', params: checked});
            }
        }
    }
    //
    // SPECTATOR CONTROLS END
    //

    //
    // FUNCTION DEFINITIONS START
    //
    //
    // Function Name: fromScript()
    //
    // Relevant Variables:
    // None
    //
    // Arguments:
    // message: The message sent from the SpectatorCamera JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from spectatorCamera.js.
    //
    function fromScript(message) {
        switch (message.method) {
        case 'updateSpectatorCameraCheckbox':
            cameraToggleCheckBox.checked = message.params;
        break;
        case 'updateMonitorShowsSwitch':
            monitorShowsSwitch.checked = message.params;
        break;
        case 'updateControllerMappingCheckbox':
            switchViewFromControllerCheckBox.checked = message.setting;
            switchViewFromControllerCheckBox.enabled = true;
            if (message.controller === "OculusTouch") {
                switchViewFromControllerCheckBox.text = "Clicking Touch's Left Thumbstick Switches Monitor View";
            } else if (message.controller === "Vive") {
                switchViewFromControllerCheckBox.text = "Clicking Left Thumb Pad Switches Monitor View";
            } else {
                switchViewFromControllerCheckBox.text = "Pressing Ctrl+0 Switches Monitor View";
                switchViewFromControllerCheckBox.checked = true;
                switchViewFromControllerCheckBox.enabled = false;
            }
        break;
        case 'showPreviewTextureNotInstructions':
            console.log('showPreviewTextureNotInstructions recvd', JSON.stringify(message));
            spectatorCameraPreview.url = message.url;
            spectatorCameraPreview.visible = message.setting;
        break;
        default:
            console.log('Unrecognized message from spectatorCamera.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
