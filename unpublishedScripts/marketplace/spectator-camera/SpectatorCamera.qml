//
//  SpectatorCamera.qml
//  qml/hifi
//
//  Spectator Camera v2.0
//
//  Created by Zach Fox on 2018-04-18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit
import "qrc:////qml//controls" as HifiControls
import "qrc:////qml//hifi" as Hifi

Rectangle {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    property bool uiReady: false;
    property bool processingStillSnapshot: false;
    property bool processing360Snapshot: false;
    // Style
    color: "#404040";

    // The letterbox used for popup messages
    Hifi.LetterboxMessage {
        id: letterboxMessage;
        z: 998; // Force the popup on top of everything else
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
    Rectangle {
        id: titleBarContainer;
        // Size
        width: root.width;
        height: 60;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;
        color: "#121212";

        // "Spectator" text
        HifiStylesUit.RalewaySemiBold {
            id: titleBarText;
            text: "Spectator Camera 2.3";
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            width: paintedWidth;
            height: parent.height;
            size: 22;
            // Style
            color: hifi.colors.white;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        Switch {
            id: masterSwitch;
            focusPolicy: Qt.ClickFocus;
            width: 65;
            height: 30;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            hoverEnabled: true;

            onHoveredChanged: {
                if (hovered) {
                    switchHandle.color = hifi.colors.blueHighlight;
                } else {
                    switchHandle.color = hifi.colors.lightGray;
                }
            }

            onClicked: {
                if (!checked) {
                    flashCheckBox.checked = false;
                }
                sendToScript({method: (checked ? 'spectatorCameraOn' : 'spectatorCameraOff')});
                sendToScript({method: 'updateCameravFoV', vFoV: fieldOfViewSlider.value});
            }

            background: Rectangle {
                color: parent.checked ? "#1FC6A6" : hifi.colors.white;
                implicitWidth: masterSwitch.width;
                implicitHeight: masterSwitch.height;
                radius: height/2;
            }

            indicator: Rectangle {
                id: switchHandle;
                implicitWidth: masterSwitch.height - 4;
                implicitHeight: implicitWidth;
                radius: implicitWidth/2;
                border.color: "#E3E3E3";
                color: "#404040";
                x: Math.max(4, Math.min(parent.width - width - 4, parent.visualPosition * parent.width - (width / 2) - 4))
                y: parent.height / 2 - height / 2;
                Behavior on x {
                    enabled: !masterSwitch.down
                    SmoothedAnimation { velocity: 200 }
                }

            }
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        z: 999;
        id: processingSnapshot;
        anchors.fill: parent;
        visible: root.processing360Snapshot || !root.uiReady;
        color: Qt.rgba(0.0, 0.0, 0.0, 0.85);        

        // This object is always used in a popup.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            propagateComposedEvents: false;
        }
                
        AnimatedImage {
            id: processingImage;
            source: "processing.gif"
            width: 74;
            height: width;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
        }

        HifiStylesUit.RalewaySemiBold {
            text: root.uiReady ? "Processing..." : "";
            // Anchors
            anchors.top: processingImage.bottom;
            anchors.topMargin: 4;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: paintedWidth;
            // Text size
            size: 26;
            // Style
            color: hifi.colors.white;
            verticalAlignment: Text.AlignVCenter;
        }
    }

    //
    // SPECTATOR CONTROLS START
    //
    Item {
        id: spectatorControlsContainer;
        // Anchors
        anchors.top: titleBarContainer.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        // Instructions or Preview
        Rectangle {
            id: spectatorCameraImageContainer;
            anchors.left: parent.left;
            anchors.top: parent.top;
            anchors.right: parent.right;
            height: 250;
            color: masterSwitch.checked ? "transparent" : "black";

            AnimatedImage {
                source: "static.gif"
                visible: !masterSwitch.checked;
                anchors.fill: parent;
                opacity: 0.15;
            }

            // Instructions (visible when display texture isn't set)
            HifiStylesUit.FiraSansRegular {
                id: spectatorCameraInstructions;
                text: "Turn on Spectator Camera for a preview\nof " + (HMD.active ? "what your monitor shows." : "the camera's view.");
                size: 16;
                color: hifi.colors.white;
                visible: !masterSwitch.checked;
                anchors.fill: parent;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }

            HifiStylesUit.FiraSansRegular {
                text: ":)";
                size: 28;
                color: hifi.colors.white;
                visible: root.processing360Snapshot || root.processingStillSnapshot;
                anchors.fill: parent;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }

            // Spectator Camera Preview
            Hifi.ResourceImageItem {
                id: spectatorCameraPreview;
                visible: masterSwitch.checked && !root.processing360Snapshot && !root.processingStillSnapshot;
                url: showCameraView.checked || !HMD.active ? "resource://spectatorCameraFrame" : "resource://hmdPreviewFrame";
                ready: masterSwitch.checked;
                mirrorVertically: true;
                anchors.fill: parent;
                onVisibleChanged: {
                    ready = masterSwitch.checked;
                    update();
                }
            }

            Item {
                visible: HMD.active;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 40;

                LinearGradient {
                    anchors.fill: parent;
                    start: Qt.point(0, 0);
                    end: Qt.point(0, height);
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: hifi.colors.black }
                        GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0) }
                    }
                }

                HifiStylesUit.HiFiGlyphs {
                    id: monitorShowsSwitchLabelGlyph;
                    text: hifi.glyphs.screen;
                    size: 32;
                    color: hifi.colors.white;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    anchors.left: parent.left;
                    anchors.leftMargin: 16;
                }
                HifiStylesUit.RalewayLight {
                    id: monitorShowsSwitchLabel;
                    text: "Monitor View:";
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    anchors.left: monitorShowsSwitchLabelGlyph.right;
                    anchors.leftMargin: 8;
                    size: 20;
                    width: paintedWidth;
                    height: parent.height;
                    color: hifi.colors.white;
                    verticalAlignment: Text.AlignVCenter;
                }
                Item {
                    anchors.left: monitorShowsSwitchLabel.right;
                    anchors.leftMargin: 14;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;

                    HifiControlsUit.RadioButton {
                        id: showCameraView;
                        text: "Camera View";
                        width: 125;
                        anchors.left: parent.left;
                        anchors.leftMargin: 10;
                        anchors.verticalCenter: parent.verticalCenter;
                        colorScheme: hifi.colorSchemes.dark;
                        onClicked: {
                            if (showHmdPreview.checked) {
                                showHmdPreview.checked = false;
                            }
                            if (!showCameraView.checked && !showHmdPreview.checked) {
                                showCameraView.checked = true;
                            }
                        }
                        onCheckedChanged: {
                            if (checked) {
                                sendToScript({method: 'setMonitorShowsCameraView', params: true});
                            }
                        }
                    }
        
                    HifiControlsUit.RadioButton {
                        id: showHmdPreview;
                        text: "VR Preview";
                        anchors.left: showCameraView.right;
                        anchors.leftMargin: 10;
                        width: 125;
                        anchors.verticalCenter: parent.verticalCenter;
                        colorScheme: hifi.colorSchemes.dark;
                        onClicked: {
                            if (showCameraView.checked) {
                                showCameraView.checked = false;
                            }
                            if (!showCameraView.checked && !showHmdPreview.checked) {
                                showHmdPreview.checked = true;
                            }
                        }
                        onCheckedChanged: {
                            if (checked) {
                                sendToScript({method: 'setMonitorShowsCameraView', params: false});
                            }
                        }
                    }
                }
            }
            
            HifiStylesUit.HiFiGlyphs {
                id: flashGlyph;
                visible: flashCheckBox.visible;
                text: hifi.glyphs.lightning;
                size: 26;
                color: hifi.colors.white;
                anchors.verticalCenter: flashCheckBox.verticalCenter;
                anchors.right: flashCheckBox.left;
                anchors.rightMargin: -2;
            }
            HifiControlsUit.CheckBox {
                id: flashCheckBox;
                visible: masterSwitch.checked;
                color: hifi.colors.white;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: takeSnapshotButton.left;
                anchors.rightMargin: -8;
                anchors.verticalCenter: takeSnapshotButton.verticalCenter;
                boxSize: 22;
                onClicked: {
                    sendToScript({method: 'setFlashStatus', enabled: checked});
                }
            }
            HifiControlsUit.Button {
                id: takeSnapshotButton;
                enabled: masterSwitch.checked;
                text: "SNAP PICTURE";
                colorScheme: hifi.colorSchemes.light;
                color: hifi.buttons.white;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 8;
                anchors.right: take360SnapshotButton.left;
                anchors.rightMargin: 12;
                width: 135;
                height: 35;
                onClicked: {
                    root.processingStillSnapshot = true;
                    sendToScript({method: 'takeSecondaryCameraSnapshot'});
                }
            }
            HifiControlsUit.Button {
                id: take360SnapshotButton;
                enabled: masterSwitch.checked;
                text: "SNAP 360";
                colorScheme: hifi.colorSchemes.light;
                color: hifi.buttons.white;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 8;
                anchors.right: parent.right;
                anchors.rightMargin: 12;
                width: 135;
                height: 35;
                onClicked: {
                    root.processing360Snapshot = true;
                    sendToScript({method: 'takeSecondaryCamera360Snapshot'});
                }
            }
        }

        Item {
            anchors.top: spectatorCameraImageContainer.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 26;
            anchors.right: parent.right;
            anchors.rightMargin: 26;
            anchors.bottom: parent.bottom;

            Item {
                id: fieldOfView;
                visible: masterSwitch.checked;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 35;

                HifiStylesUit.RalewaySemiBold {
                    id: fieldOfViewLabel;
                    text: "Field of View (" + fieldOfViewSlider.value + "\u00B0): ";
                    size: 20;
                    color: hifi.colors.white;
                    anchors.left: parent.left;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    width: 172;
                    horizontalAlignment: Text.AlignLeft;
                    verticalAlignment: Text.AlignVCenter;
                }

                HifiControlsUit.Slider {
                    id: fieldOfViewSlider;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    anchors.right: resetvFoV.left;
                    anchors.rightMargin: 8;
                    anchors.left: fieldOfViewLabel.right;
                    anchors.leftMargin: 8;
                    colorScheme: hifi.colorSchemes.dark;
                    from: 10.0;
                    to: 120.0;
                    value: 45.0;
                    stepSize: 1;

                    onValueChanged: {
                        sendToScript({method: 'updateCameravFoV', vFoV: value});
                    }
                    onPressedChanged: {
                        if (!pressed) {
                            sendToScript({method: 'updateCameravFoV', vFoV: value});
                        }
                    }
                }

                HifiControlsUit.GlyphButton {
                    id: resetvFoV;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.right: parent.right;
                    anchors.rightMargin: -8;
                    height: parent.height - 8;
                    width: height;
                    glyph: hifi.glyphs.reload;
                    onClicked: {
                        fieldOfViewSlider.value = 45.0;
                    }
                }
            }

            Item {
                visible: HMD.active;
                anchors.top: fieldOfView.bottom;
                anchors.topMargin: 18;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: childrenRect.height;

                HifiStylesUit.RalewaySemiBold {
                    id: shortcutsHeaderText;
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    height: paintedHeight;
                    text: "Shortcuts";
                    size: 20;
                    color: hifi.colors.white;
                }

                // "Switch View From Controller" Checkbox
                HifiControlsUit.CheckBox {
                    id: switchViewFromControllerCheckBox;
                    color: hifi.colors.white;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.left: parent.left;
                    anchors.top: shortcutsHeaderText.bottom;
                    anchors.topMargin: 8;
                    text: "";
                    labelFontSize: 20;
                    labelFontWeight: Font.Normal;
                    boxSize: 24;
                    onClicked: {
                        sendToScript({method: 'changeSwitchViewFromControllerPreference', params: checked});
                    }
                }

                // "Take Snapshot" Checkbox
                HifiControlsUit.CheckBox {
                    id: takeSnapshotFromControllerCheckBox;
                    color: hifi.colors.white;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.left: parent.left; 
                    anchors.top: switchViewFromControllerCheckBox.bottom;
                    anchors.topMargin: 4;
                    text: "";
                    labelFontSize: 20;
                    labelFontWeight: Font.Normal;
                    boxSize: 24;
                    onClicked: {
                        sendToScript({method: 'changeTakeSnapshotFromControllerPreference', params: checked});
                    }
                }
            }

            HifiControlsUit.Button {
                text: "Change Snapshot Location";
                colorScheme: hifi.colorSchemes.dark;
                color: hifi.buttons.none;
                anchors.bottom: spectatorDescriptionContainer.top;
                anchors.bottomMargin: 16;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 35;
                onClicked: {
                    sendToScript({method: 'openSettings'});
                }
            }

            Item {
                id: spectatorDescriptionContainer;
                // Size
                height: childrenRect.height;
                // Anchors
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 20;

                // "Spectator" app description text
                HifiStylesUit.RalewayRegular {
                    id: spectatorDescriptionText;
                    text: "While you're using a VR headset, you can use this app to change what your monitor shows. " +
                    "Try it when streaming or recording video.";
                    // Text size
                    size: 20;
                    // Size
                    height: paintedHeight;
                    // Anchors
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    anchors.top: parent.top;
                    // Style
                    color: hifi.colors.white;
                    wrapMode: Text.Wrap;
                    // Alignment
                    horizontalAlignment: Text.AlignHLeft;
                    verticalAlignment: Text.AlignVCenter;
                }

                // "Learn More" text
                HifiStylesUit.RalewayRegular {
                    id: spectatorLearnMoreText;
                    text: "Learn More About Spectator";
                    // Text size
                    size: 20;
                    // Size
                    width: paintedWidth;
                    height: paintedHeight;
                    // Anchors
                    anchors.top: spectatorDescriptionText.bottom;
                    anchors.topMargin: 10;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
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
                                '<font size="4"><a href="https://obsproject.com/forum/threads/official-overview-guide.402/">OBS Official Overview Guide</a></font><br><br>' +
                                '<b>Snapshots</b> taken using Spectator Camera will be saved in your Snapshots Directory - change via Settings -> General.');
                        }
                        onEntered: parent.color = hifi.colors.blueHighlight;
                        onExited: parent.color = hifi.colors.blueAccent;
                    }
                }
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
        case 'initializeUI':
            masterSwitch.checked = message.masterSwitchOn;
            flashCheckBox.checked = message.flashCheckboxChecked;
            showCameraView.checked = message.monitorShowsCamView;
            showHmdPreview.checked = !message.monitorShowsCamView;
            root.uiReady = true;
        break;
        case 'updateMonitorShowsSwitch':
            showCameraView.checked = message.params;
            showHmdPreview.checked = !message.params;
        break;
        case 'updateControllerMappingCheckbox':
            switchViewFromControllerCheckBox.checked = message.switchViewSetting;
            switchViewFromControllerCheckBox.enabled = true;
            takeSnapshotFromControllerCheckBox.checked = message.takeSnapshotSetting;
            takeSnapshotFromControllerCheckBox.enabled = true;

            if (message.controller === "OculusTouch") {
                switchViewFromControllerCheckBox.text = "Left Thumbstick: Switch Monitor View";
                takeSnapshotFromControllerCheckBox.text = "Right Thumbstick: Take Snapshot";
            } else if (message.controller === "Vive") {
                switchViewFromControllerCheckBox.text = "Left Thumb Pad: Switch Monitor View";
                takeSnapshotFromControllerCheckBox.text = "Right Thumb Pad: Take Snapshot";
            } else {
                switchViewFromControllerCheckBox.text = "Pressing Ctrl+0 Switches Monitor View";
                switchViewFromControllerCheckBox.checked = true;
                switchViewFromControllerCheckBox.enabled = false;
                takeSnapshotFromControllerCheckBox.visible = false;
            }
        break;
        case 'finishedProcessing360Snapshot':
            root.processing360Snapshot = false;
        break;
        case 'startedProcessingStillSnapshot':
            root.processingStillSnapshot = true;
        break;
        case 'finishedProcessingStillSnapshot':
            root.processingStillSnapshot = false;
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
