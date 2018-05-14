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
import "qrc:////qml//styles-uit" as HifiStylesUit
import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//controls" as HifiControls
import "qrc:////qml//hifi" as Hifi

Rectangle {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
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
            text: "Spectator Camera";
            // Text size
            size: hifi.fontSizes.overlayTitle;
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

        HifiControlsUit.Switch {
            id: masterSwitch;
            width: 65;
            height: parent.height;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            onCheckedChanged: {
                sendToScript({method: (checked ? 'spectatorCameraOn' : 'spectatorCameraOff')});
                sendToScript({method: 'updateCameravFoV', vFoV: fieldOfViewSlider.value});
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
        visible: root.processing360Snapshot;
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
            text: "Processing...";
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
        // Size
        height: root.height - spectatorDescriptionContainer.height - titleBarContainer.height;
        // Anchors
        anchors.top: titleBarContainer.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        // Instructions or Preview
        Rectangle {
            id: spectatorCameraImageContainer;
            anchors.left: parent.left;
            anchors.top: cameraToggleButton.bottom;
            anchors.topMargin: 8;
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
                color: hifi.colors.lightGrayText;
                visible: !masterSwitch.checked;
                anchors.fill: parent;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }

            // Spectator Camera Preview
            Hifi.ResourceImageItem {
                id: spectatorCameraPreview;
                visible: masterSwitch.checked;
                url: monitorShowsSwitch.checked || !HMD.active ? "resource://spectatorCameraFrame" : "resource://hmdPreviewFrame";
                ready: masterSwitch.checked;
                mirrorVertically: true;
                anchors.fill: parent;
                onVisibleChanged: {
                    ready = masterSwitch.checked;
                    update();
                }
            }

            Item {
                visible: true//HMD.active;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;

                // "Monitor Shows" Switch Label Glyph
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
                // "Monitor Shows" Switch Label
                HifiStylesUit.RalewayLight {
                    id: monitorShowsSwitchLabel;
                    text: "MONITOR SHOWS:";
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
                    anchors.leftMargin: 10;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10;
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;

			        HifiControlsUit.RadioButton {
				        id: showCameraView;
				        text: "Camera View";
				        width: 140;
                        anchors.left: parent.left;
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
				        text: "HMD";
				        anchors.left: showCameraView.right;
				        anchors.leftMargin: 10;
                        anchors.right: parent.right;
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
        }

        Item {
            id: fieldOfView;
            anchors.top: spectatorCameraImageContainer.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.right: parent.right;
            height: 35;

            HifiStylesUit.FiraSansRegular {
                id: fieldOfViewLabel;
                text: "Field of View (" + fieldOfViewSlider.value + "\u00B0): ";
                size: 16;
                color: hifi.colors.lightGrayText;
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                width: 140;
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
                anchors.rightMargin: 6;
                height: parent.height - 8;
                width: height;
                glyph: hifi.glyphs.reload;
                onClicked: {
                    fieldOfViewSlider.value = 45.0;
                }
            }
        }

        // "Switch View From Controller" Checkbox
        HifiControlsUit.CheckBox {
            id: switchViewFromControllerCheckBox;
            visible: HMD.active;
            colorScheme: hifi.colorSchemes.dark;
            anchors.left: parent.left;
            anchors.top: monitorShowsSwitch.bottom;
            anchors.topMargin: 14;
            text: "";
            boxSize: 24;
            onClicked: {
                sendToScript({method: 'changeSwitchViewFromControllerPreference', params: checked});
            }
        }

        // "Take Snapshot" Checkbox
        HifiControlsUit.CheckBox {
            id: takeSnapshotFromControllerCheckBox;
            visible: HMD.active;
            colorScheme: hifi.colorSchemes.dark;
            anchors.left: parent.left;
            anchors.top: switchViewFromControllerCheckBox.bottom;
            anchors.topMargin: 10;
            text: "";
            boxSize: 24;
            onClicked: {
                sendToScript({method: 'changeTakeSnapshotFromControllerPreference', params: checked});
            }
        }

		HifiControlsUit.Button {
			id: takeSnapshotButton;
            enabled: masterSwitch.checked;
            text: "Take Still Snapshot";
			colorScheme: hifi.colorSchemes.dark;
			color: hifi.buttons.blue;
			anchors.top: takeSnapshotFromControllerCheckBox.visible ? takeSnapshotFromControllerCheckBox.bottom : fieldOfView.bottom;
			anchors.topMargin: 8;
			anchors.left: parent.left;
            width: parent.width/2 - 10;
			height: 40;
			onClicked: {
				sendToScript({method: 'takeSecondaryCameraSnapshot'});
			}
		}
		HifiControlsUit.Button {
			id: take360SnapshotButton;
            enabled: masterSwitch.checked;
            text: "Take 360 Snapshot";
			colorScheme: hifi.colorSchemes.dark;
			color: hifi.buttons.blue;
			anchors.top: takeSnapshotFromControllerCheckBox.visible ? takeSnapshotFromControllerCheckBox.bottom : fieldOfView.bottom;
			anchors.topMargin: 8;
			anchors.right: parent.right;
            width: parent.width/2 - 10;
			height: 40;
			onClicked: {
                root.processing360Snapshot = true;
				sendToScript({method: 'takeSecondaryCamera360Snapshot'});
			}
		}
    }
    //
    // SPECTATOR CONTROLS END
    //

    //
    // SPECTATOR APP DESCRIPTION START
    //
    Item {
        id: spectatorDescriptionContainer;
        // Size
        width: root.width;
        height: childrenRect.height;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: anchors.bottom;

        // "Spectator" app description text
        HifiStylesUit.RalewayLight {
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
            masterSwitch.checked = message.params;
        break;
        case 'updateMonitorShowsSwitch':
            monitorShowsSwitch.checked = message.params;
        break;
        case 'updateControllerMappingCheckbox':
            switchViewFromControllerCheckBox.checked = message.switchViewSetting;
            switchViewFromControllerCheckBox.enabled = true;
            takeSnapshotFromControllerCheckBox.checked = message.takeSnapshotSetting;
            takeSnapshotFromControllerCheckBox.enabled = true;

            if (message.controller === "OculusTouch") {
                switchViewFromControllerCheckBox.text = "Clicking Touch's Left Thumbstick Switches Monitor View";
				takeSnapshotFromControllerCheckBox.text = "Clicking Touch's Right Thumbstick Takes Snapshot";
            } else if (message.controller === "Vive") {
                switchViewFromControllerCheckBox.text = "Clicking Left Thumb Pad Switches Monitor View";
				takeSnapshotFromControllerCheckBox.text = "Clicking Right Thumb Pad Takes Snapshot";
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
        default:
            console.log('Unrecognized message from spectatorCamera.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
