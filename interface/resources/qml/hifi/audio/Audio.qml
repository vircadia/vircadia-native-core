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
import controlsUit 1.0 as HifiControlsUit
import "../../windows"
import "./" as AudioControls

Rectangle {
    id: root;

    HifiConstants { id: hifi; }

    property var eventBridge;
    property string title: "Audio Settings"
    property int switchHeight: 16
    property int switchWidth: 40
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"
    function showTitle() {
        return (root.parent !== null) && root.parent.objectName == "loader";
    }


    property bool isVR: AudioScriptingInterface.context === "VR"
    property real rightMostInputLevelPos: 450
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

        RowLayout {
            x: 2 * margins.paddings;
            spacing: columnOne.width;
            width: parent.width;

            // mute is in its own row
            ColumnLayout {
                id: columnOne
                spacing: 12;
                x: margins.paddings
                HifiControlsUit.Switch {
                    id: muteMic;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn: "Mute microphone";
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.muted;
                    onCheckedChanged: {
                        AudioScriptingInterface.muted = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.muted; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: stereoInput;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn:  qsTr("Stereo input");
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.isStereoInput;
                    onCheckedChanged: {
                        AudioScriptingInterface.isStereoInput = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.isStereoInput; }); // restore binding
                    }
                }
            }

            ColumnLayout {
                spacing: 12;
                HifiControlsUit.Switch {
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn: "Noise Reduction";
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.noiseReduction;
                    onCheckedChanged: {
                        AudioScriptingInterface.noiseReduction = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.noiseReduction; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: audioLevelSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn: qsTr("Audio Level Meter");
                    backgroundOnColor: "#E3E3E3";
                    checked: AvatarInputs.showAudioTools;
                    onCheckedChanged: {
                        AvatarInputs.showAudioTools = checked;
                        checked = Qt.binding(function() { return AvatarInputs.showAudioTools; }); // restore binding
                    }
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
                color: hifi.colors.white;
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
                color: hifi.colors.white;
                text: qsTr("Choose input device");
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
                    checked: bar.currentIndex === 0 ? selectedDesktop : selectedHMD;
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
                AudioControls.InputPeak {
                    id: inputLevel
                    anchors.right: parent.right
                    peak: model.peak;
                    anchors.verticalCenter: parent.verticalCenter
                    visible: ((bar.currentIndex === 1 && isVR) ||
                             (bar.currentIndex === 0 && !isVR)) &&
                             AudioScriptingInterface.devices.input.peakValuesAvailable;
                }
            }
            Component.onCompleted: {
                console.log("width " + rightMostInputLevelPos);
            }
        }
        AudioControls.LoopbackAudio {
            x: margins.paddings

            visible: (bar.currentIndex === 1 && isVR) ||
                (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
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
                color: hifi.colors.white;
                size: 36;
            }

            RalewayRegular {
                width: margins.sizeText + margins.sizeLevel
                anchors.left: parent.left
                anchors.leftMargin: margins.sizeCheckBox
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.white;
                text: qsTr("Choose output device");
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
        AudioControls.PlaySampleSound {
            x: margins.paddings

            visible: (bar.currentIndex === 1 && isVR) ||
                     (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
        }
    }
}
