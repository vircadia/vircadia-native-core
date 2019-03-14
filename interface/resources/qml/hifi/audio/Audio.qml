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

import QtQuick 2.10
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
    // leave as blank, this is user's volume for the avatar mixer
    property var myAvatarUuid: ""
    property string title: "Audio Settings"
    property int switchHeight: 16
    property int switchWidth: 40
    readonly property real verticalScrollWidth: 10
    readonly property real verticalScrollShaft: 8
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"
    function showTitle() {
        return (root.parent !== null) && root.parent.objectName == "loader";
    }


    property bool isVR: AudioScriptingInterface.context === "VR"
    property real rightMostInputLevelPos: 440
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
        height: 28;
        currentIndex: isVR ? 1 : 0;

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

    function updateMyAvatarGainFromQML(sliderValue, isReleased) {
        if (Users.getAvatarGain(myAvatarUuid) != sliderValue) {
            Users.setAvatarGain(myAvatarUuid, sliderValue);
        }
    }

    Component.onCompleted: enablePeakValues();

    Flickable {
        id: flickView;
        anchors.top: bar.bottom;
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        width: parent.width;
        contentWidth: parent.width;
        contentHeight: contentItem.childrenRect.height;
        boundsBehavior: Flickable.DragOverBounds;
        flickableDirection: Flickable.VerticalFlick;
        property bool isScrolling: (contentHeight - height) > 10 ? true : false;
        clip: true;

        ScrollBar.vertical: ScrollBar {
            policy: flickView.isScrolling ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff;
            parent: flickView.parent;
            anchors.top: flickView.top;
            anchors.right: flickView.right;
            anchors.bottom: flickView.bottom;
            anchors.rightMargin: -verticalScrollWidth; //compensate flickView's right margin
            background: Item {
                implicitWidth: verticalScrollWidth;
                Rectangle {
                    color: hifi.colors.darkGray30;
                    radius: 4;
                    anchors {
                        fill: parent;
                        topMargin: -1;  // Finesse size
                        bottomMargin: -2;
                    }
                }
            }
            contentItem: Item {
                implicitWidth: verticalScrollShaft;
                Rectangle {
                    radius: verticalScrollShaft/2;
                    color: hifi.colors.white30;
                    anchors {
                        fill: parent;
                        leftMargin: 2;  // Finesse size and position.
                        topMargin: 1;
                        bottomMargin: 1;
                    }
                }
            }
        }

        Separator {
            id: firstSeparator;
            anchors.top: parent.top;
        }

        Item {
            id: switchesContainer;
            x: 2 * margins.paddings;
            width: parent.width;
            // switch heights + 2 * top margins
            height: (root.switchHeight) * 3 + 48;
            anchors.top: firstSeparator.bottom;
            anchors.topMargin: 10;

            // mute is in its own row
           Item {
                id: switchContainer;
                x: margins.paddings;
                width: parent.width / 2;
                height: parent.height;
                anchors.left: parent.left;
                HifiControlsUit.Switch {
                    id: muteMic;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn: "Mute microphone";
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.muted;
                    onClicked: {
                        if (AudioScriptingInterface.pushToTalk && !checked) {
                            // disable push to talk if unmuting
                            AudioScriptingInterface.pushToTalk = false;
                        }
                        AudioScriptingInterface.muted = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.muted; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: noiseReductionSwitch;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: muteMic.bottom;
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: "Noise Reduction";
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.noiseReduction;
                    onCheckedChanged: {
                        AudioScriptingInterface.noiseReduction = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.noiseReduction; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: pttSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: noiseReductionSwitch.bottom
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: qsTr("Push To Talk (T)");
                    backgroundOnColor: "#E3E3E3";
                    checked: (bar.currentIndex === 0) ? AudioScriptingInterface.pushToTalkDesktop : AudioScriptingInterface.pushToTalkHMD;
                    onCheckedChanged: {
                        if (bar.currentIndex === 0) {
                            AudioScriptingInterface.pushToTalkDesktop = checked;
                        } else {
                            AudioScriptingInterface.pushToTalkHMD = checked;
                        }
                        checked = Qt.binding(function() {
                            if (bar.currentIndex === 0) {
                                return AudioScriptingInterface.pushToTalkDesktop;
                            } else {
                                return AudioScriptingInterface.pushToTalkHMD;
                            }
                        }); // restore binding
                    }
                }
            }

            Item {
                id: additionalSwitchContainer
                width: switchContainer.width - margins.paddings;
                height: parent.height;
                anchors.top: parent.top
                anchors.left: switchContainer.right;
                HifiControlsUit.Switch {
                    id: warnMutedSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: parent.top
                    anchors.left: parent.left
                    labelTextOn: qsTr("Warn when muted");
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.warnWhenMuted;
                    onClicked: {
                        AudioScriptingInterface.warnWhenMuted = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.warnWhenMuted; }); // restore binding
                    }
                }


                HifiControlsUit.Switch {
                    id: audioLevelSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: warnMutedSwitch.bottom
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: qsTr("Audio Level Meter");
                    backgroundOnColor: "#E3E3E3";
                    checked: AvatarInputs.showAudioTools;
                    onCheckedChanged: {
                        AvatarInputs.showAudioTools = checked;
                        checked = Qt.binding(function() { return AvatarInputs.showAudioTools; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: stereoInput;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: audioLevelSwitch.bottom
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn:  qsTr("Stereo input");
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.isStereoInput;
                    onCheckedChanged: {
                        AudioScriptingInterface.isStereoInput = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.isStereoInput; }); // restore binding
                    }
                }

            }
        }

        Item {
            id: pttTextContainer
            anchors.top: switchesContainer.bottom;
            anchors.topMargin: 10;
            anchors.left: parent.left;
            width: rightMostInputLevelPos;
            height: pttText.height;
            RalewayRegular {
                id: pttText;
                x: margins.paddings;
                color: hifi.colors.white;
                width: rightMostInputLevelPos;
                height: paintedHeight;
                wrapMode: Text.WordWrap;
                font.italic: true;
                size: 16;

                text: (bar.currentIndex === 0) ? qsTr("Press and hold the button \"T\" to talk.") :
                                qsTr("Press and hold grip triggers on both of your controllers to talk.");
            }
        }

        Separator {
            id: secondSeparator;
            anchors.top: pttTextContainer.bottom;
            anchors.topMargin: 10;
        }


        Item {
            id: inputDeviceHeader
            x: margins.paddings;
            width: parent.width - margins.paddings*2;
            height: 36;
            anchors.top: secondSeparator.bottom;
            anchors.topMargin: 10;

            HiFiGlyphs {
                width: margins.sizeCheckBox;
                text: hifi.glyphs.mic;
                color: hifi.colors.white;
                anchors.left: parent.left;
                anchors.leftMargin: -size/4; //the glyph has empty space at left about 25%
                anchors.verticalCenter: parent.verticalCenter;
                size: 30;
            }
            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter;
                width: margins.sizeText + margins.sizeLevel;
                anchors.left: parent.left;
                anchors.leftMargin: margins.sizeCheckBox;
                size: 16;
                color: hifi.colors.white;
                text: qsTr("Choose input device");
            }
        }

        ListView {
            id: inputView;
            width: parent.width - margins.paddings*2;
            anchors.top: inputDeviceHeader.bottom;
            anchors.topMargin: 10;
            x: margins.paddings
            height: contentHeight;
            spacing: 4;
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
                            stereoInput.checked = false;
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
        }
        AudioControls.LoopbackAudio {
            id: loopbackAudio
            x: margins.paddings
            anchors.top: inputView.bottom;
            anchors.topMargin: 10;

            visible: (bar.currentIndex === 1 && isVR) ||
                (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
        }

        Separator {
            id: thirdSeparator;
            anchors.top: loopbackAudio.bottom;
            anchors.topMargin: 10;
        }

        Item {
            id: outputDeviceHeader;
            anchors.topMargin: 10;
            anchors.top: thirdSeparator.bottom;
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
            height: contentHeight;
            anchors.top: outputDeviceHeader.bottom;
            anchors.topMargin: 10;
            spacing: 4;
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

        Item {
            id: gainContainer
            x: margins.paddings;
            anchors.top: outputView.bottom;
            anchors.topMargin: 10;
            width: parent.width - margins.paddings*2
            height: gainSliderTextMetrics.height

            HifiControlsUit.Slider {
                id: gainSlider
                anchors.right: parent.right
                height: parent.height
                width: 200
                minimumValue: -60.0
                maximumValue: 20.0
                stepSize: 5
                value: Users.getAvatarGain(myAvatarUuid)
                onValueChanged: {
                    updateMyAvatarGainFromQML(value, false);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateMyAvatarGainFromQML(value, false);
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: {
                        // Do nothing.
                    }
                    onDoubleClicked: {
                        gainSlider.value = 0.0
                    }
                    onPressed: {
                        // Pass through to Slider
                        mouse.accepted = false
                    }
                    onReleased: {
                        // the above mouse.accepted seems to make this
                        // never get called, nonetheless...
                        mouse.accepted = false
                    }
                }
            }
            TextMetrics {
                id: gainSliderTextMetrics
                text: gainSliderText.text
                font: gainSliderText.font
            }
            RalewayRegular {
                // The slider for my card is special, it controls the master gain
                id: gainSliderText;
                text: "Avatar volume";
                size: 16;
                anchors.left: parent.left;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
        }

        AudioControls.PlaySampleSound {
            id: playSampleSound
            x: margins.paddings
            anchors.top: gainContainer.bottom;
            anchors.topMargin: 10;

            visible: (bar.currentIndex === 1 && isVR) ||
                     (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
        }
    }
}
