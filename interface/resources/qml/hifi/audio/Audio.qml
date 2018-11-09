//
//  Audio.qml
//  qml/hifi/audio
//
//  Audio setup
//
//  Created by Vlad Stelmahovsky on 03/22/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import "../../windows"
import "./" as AudioControls

Rectangle {
    id: root;

    HifiConstants { id: hifi; }

    property var eventBridge;
    property string title: "Audio Settings"
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"
    function showTitle() {
        return (root.parent !== null) && root.parent.objectName == "loader";
    }


    property bool isVR: AudioScriptingInterface.context === "VR"
    property real rightMostInputLevelPos: 0
    //placeholder for control sizes and paddings
    //recalculates dynamically in case of UI size is changed
    QtObject {
        id: margins
        property real paddings: root.width / 20.25

        property real sizeCheckBox: root.width / 13.5
        property real sizeText: root.width / 2.5
        property real sizeLevel: root.width / 5.8
        property real sizeDesktop: root.width / 5.8
        property real sizeVR: root.width / 13.5
    }

    TabBar {
        id: bar
        spacing: 0
        width: parent.width
        height: 42
        currentIndex: isVR ? 1 : 0

        AudioControls.AudioTabButton {
            height: parent.height
            text: qsTr("Desktop")
        }
        AudioControls.AudioTabButton {
            height: parent.height
            text: qsTr("VR")
        }
    }

    property bool showPeaks: true;

    function enablePeakValues() {
        AudioScriptingInterface.devices.input.peakValuesEnabled = true;
        AudioScriptingInterface.devices.input.peakValuesEnabledChanged.connect(function(enabled) {
            if (!enabled && root.showPeaks) {
                AudioScriptingInterface.devices.input.peakValuesEnabled = true;
            }
        });
    }

    function disablePeakValues() {
        root.showPeaks = false;
        AudioScriptingInterface.devices.input.peakValuesEnabled = false;
    }

    Component.onCompleted: enablePeakValues();
    Component.onDestruction: disablePeakValues();
    onVisibleChanged: visible ? enablePeakValues() : disablePeakValues();

    Column {
        spacing: 12;
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        width: parent.width;

        Separator { }

        RalewayRegular {
            x: margins.paddings + muteMic.boxSize + muteMic.spacing;
            size: 16;
            color: "white";
            text: qsTr("Input Device Settings")
        }

        ColumnLayout {
            x: margins.paddings;
            spacing: 16;
            width: parent.width;

            // mute is in its own row
            RowLayout {
                spacing: (margins.sizeCheckBox - 10.5) * 3;
                AudioControls.CheckBox {
                    id: muteMic
                    text: qsTr("Mute microphone");
                    spacing: margins.sizeCheckBox - boxSize
                    isRedCheck: true;
                    checked: AudioScriptingInterface.muted;
                    onClicked: {
                        AudioScriptingInterface.muted = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.muted; }); // restore binding
                    }
                }

                AudioControls.CheckBox {
                    id: stereoMic
                    spacing: muteMic.spacing;
                    text: qsTr("Enable stereo input");
                    checked: AudioScriptingInterface.isStereoInput;
                    onClicked: {
                        AudioScriptingInterface.isStereoInput = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.isStereoInput; }); // restore binding
                    }
                }
            }

            RowLayout {
                spacing: muteMic.spacing*2; //make it visually distinguish
                AudioControls.CheckBox {
                    spacing: muteMic.spacing
                    text: qsTr("Enable noise reduction");
                    checked: AudioScriptingInterface.noiseReduction;
                    onClicked: {
                        AudioScriptingInterface.noiseReduction = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.noiseReduction; }); // restore binding
                    }
                }
                AudioControls.CheckBox {
                    spacing: muteMic.spacing
                    text: qsTr("Show audio level meter");
                    checked: AvatarInputs.showAudioTools;
                    onClicked: {
                        AvatarInputs.showAudioTools = checked;
                        checked = Qt.binding(function() { return AvatarInputs.showAudioTools; }); // restore binding
                    }
                    onXChanged: rightMostInputLevelPos = x + width
                }
            }
        }

        Separator {}

        Item {
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: 36

            HiFiGlyphs {
                width: margins.sizeCheckBox
                text: hifi.glyphs.mic;
                color: hifi.colors.primaryHighlight;
                anchors.left: parent.left
                anchors.leftMargin: -size/4 //the glyph has empty space at left about 25%
                anchors.verticalCenter: parent.verticalCenter;
                size: 30;
            }
            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter;
                width: margins.sizeText + margins.sizeLevel
                anchors.left: parent.left
                anchors.leftMargin: margins.sizeCheckBox
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("CHOOSE INPUT DEVICE");
            }
        }

        ListView {
            id: inputView
            width: parent.width - margins.paddings*2
            x: margins.paddings
            height: Math.min(150, contentHeight);
            spacing: 4;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: AudioScriptingInterface.devices.input;
            delegate: Item {
                width: rightMostInputLevelPos
                height: margins.sizeCheckBox > checkBoxInput.implicitHeight ?
                            margins.sizeCheckBox : checkBoxInput.implicitHeight

                AudioControls.CheckBox {
                    id: checkBoxInput
                    anchors.left: parent.left
                    spacing: margins.sizeCheckBox - boxSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - inputLevel.width
                    clip: true
                    checkable: !checked
                    checked: bar.currentIndex === 0 ? selectedDesktop :  selectedHMD;
                    boxSize: margins.sizeCheckBox / 2
                    isRound: true
                    text: devicename
                    onPressed: {
                        if (!checked) {
                            stereoMic.checked = false;
                            AudioScriptingInterface.setStereoInput(false); // the next selected audio device might not support stereo
                            AudioScriptingInterface.setInputDevice(info, bar.currentIndex === 1);
                        }
                    }
                }
                InputPeak {
                    id: inputLevel
                    anchors.right: parent.right
                    peak: model.peak;
                    anchors.verticalCenter: parent.verticalCenter
                    visible: ((bar.currentIndex === 1 && isVR) ||
                             (bar.currentIndex === 0 && !isVR)) &&
                             AudioScriptingInterface.devices.input.peakValuesAvailable;
                }
            }
        }

        Separator {}

        Item {
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: 36

            HiFiGlyphs {
                anchors.left: parent.left
                anchors.leftMargin: -size/4 //the glyph has empty space at left about 25%
                anchors.verticalCenter: parent.verticalCenter;
                width: margins.sizeCheckBox
                text: hifi.glyphs.unmuted;
                color: hifi.colors.primaryHighlight;
                size: 36;
            }

            RalewayRegular {
                width: margins.sizeText + margins.sizeLevel
                anchors.left: parent.left
                anchors.leftMargin: margins.sizeCheckBox
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("CHOOSE OUTPUT DEVICE");
            }
        }

        ListView {
            id: outputView
            width: parent.width - margins.paddings*2
            x: margins.paddings
            height: Math.min(360 - inputView.height, contentHeight);
            spacing: 4;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: AudioScriptingInterface.devices.output;
            delegate: Item {
                width: rightMostInputLevelPos
                height: margins.sizeCheckBox > checkBoxOutput.implicitHeight ?
                            margins.sizeCheckBox : checkBoxOutput.implicitHeight

                AudioControls.CheckBox {
                    id: checkBoxOutput
                    width: parent.width
                    spacing: margins.sizeCheckBox - boxSize
                    boxSize: margins.sizeCheckBox / 2
                    isRound: true
                    checked: bar.currentIndex === 0 ? selectedDesktop :  selectedHMD;
                    checkable: !checked
                    text: devicename
                    onPressed: {
                        if (!checked) {
                            AudioScriptingInterface.setOutputDevice(info, bar.currentIndex === 1);
                        }
                    }
                }
            }
        }
        PlaySampleSound {
            x: margins.paddings

            visible: (bar.currentIndex === 1 && isVR) ||
                     (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
        }
    }
}
