//
//  Audio.qml
//  qml/hifi/audio
//
//  Audio setup
//
//  Created by Vlad Stelmahovsky on 03/22/2017
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

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
    property bool pushToTalk: (bar.currentIndex === 0) ? AudioScriptingInterface.pushToTalkDesktop : AudioScriptingInterface.pushToTalkHMD;
    property bool muted: (bar.currentIndex === 0) ? AudioScriptingInterface.mutedDesktop : AudioScriptingInterface.mutedHMD;
    readonly property real verticalScrollWidth: 10
    readonly property real verticalScrollShaft: 8
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"
    function showTitle() {
        return (root.parent !== null) && root.parent.objectName == "loader";
    }

    property bool isVR: AudioScriptingInterface.context === "VR"
    property real rightMostInputLevelPos: root.width
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
        if (AudioScriptingInterface.getAvatarGain() != sliderValue) {
            AudioScriptingInterface.setAvatarGain(sliderValue);
        }
    }
    function updateInjectorGainFromQML(sliderValue, isReleased) {
        if (AudioScriptingInterface.getInjectorGain() != sliderValue) {
            AudioScriptingInterface.setInjectorGain(sliderValue);       // server side
            AudioScriptingInterface.setLocalInjectorGain(sliderValue);  // client side
        }
    }
    function updateSystemInjectorGainFromQML(sliderValue, isReleased) {
        if (AudioScriptingInterface.getSystemInjectorGain() != sliderValue) {
            AudioScriptingInterface.setSystemInjectorGain(sliderValue);
        }
    }
    function updateNoiseReductionThresholdFromQML(sliderValue) {
        if (AudioScriptingInterface.getNoiseReductionThreshold() !== sliderValue) {
            AudioScriptingInterface.setNoiseReductionThreshold(sliderValue);
        }
    }

    Component.onCompleted: {
        enablePeakValues();
    }

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
            z: 100  // Display over top of separators.

            background: Item {
                implicitWidth: verticalScrollWidth;
                Rectangle {
                    color: hifi.colors.baseGrayShadow
                    radius: 4;
                    anchors {
                        fill: parent;
                        topMargin: 2  // Finess position
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
                        topMargin: 1;  // Finesse position.
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
            height: (bar.currentIndex === 0) ? (root.switchHeight) * 2 + 48 : (root.switchHeight) * 4 + 48;
            anchors.top: firstSeparator.bottom;
            anchors.topMargin: 10;

           Item {
                id: switchContainer;
                x: margins.paddings;
                width: parent.width / 2;
                height: parent.height;
                anchors.top: parent.top
                anchors.left: parent.left;

                HifiControlsUit.Switch {
                    id: muteMic;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    labelTextOn: "Mute microphone";
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: muted;
                    onClicked: {
                        if (bar.currentIndex === 0) {
                            AudioScriptingInterface.mutedDesktop = checked;
                        }
                        else {
                            AudioScriptingInterface.mutedHMD = checked;
                        }
                    }
                }

                HifiControlsUit.Switch {
                    id: pttSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: muteMic.bottom;
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: (bar.currentIndex === 0) ? qsTr("Push To Talk (T)") : qsTr("Push To Talk");
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: (bar.currentIndex === 0) ? AudioScriptingInterface.pushToTalkDesktop : AudioScriptingInterface.pushToTalkHMD;
                    onCheckedChanged: {
                        if (bar.currentIndex === 0) {
                            AudioScriptingInterface.pushToTalkDesktop = checked;
                        } else {
                            AudioScriptingInterface.pushToTalkHMD = checked;
                        }
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
                    labelTextOn: qsTr("HMD Mute Warning");
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.warnWhenMuted;
                    visible: bar.currentIndex !== 0;
                    onClicked: {
                        AudioScriptingInterface.warnWhenMuted = checked;
                        checked = Qt.binding(function() { return AudioScriptingInterface.warnWhenMuted; }); // restore binding
                    }
                }

                HifiControlsUit.Switch {
                    id: audioLevelSwitch
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: warnMutedSwitch.visible ? warnMutedSwitch.bottom : parent.top
                    anchors.topMargin: bar.currentIndex === 0 ? 0 : 24
                    anchors.left: parent.left
                    labelTextOn: qsTr("Audio Level Meter");
                    labelTextSize: 16;
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
                    labelTextSize: 16;
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
                                qsTr("Press and hold grip triggers on both controllers to talk.");
            }
        }

        Item {
            id: avatarGainContainer
            x: margins.paddings;
            anchors.top: pttTextContainer.bottom;
            anchors.topMargin: 10;
            width: parent.width - margins.paddings*2
            height: avatarGainSliderTextMetrics.height

            HifiControlsUit.Slider {
                id: avatarGainSlider
                anchors.right: parent.right
                height: parent.height
                width: 200
                minimumValue: -60.0
                maximumValue: 20.0
                stepSize: 5
                value: AudioScriptingInterface.getAvatarGain()
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
                        avatarGainSlider.value = 0.0
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
                id: avatarGainSliderTextMetrics
                text: avatarGainSliderText.text
                font: avatarGainSliderText.font
            }
            RalewayRegular {
                // The slider for my card is special, it controls the master gain
                id: avatarGainSliderText;
                text: "People volume";
                size: 16;
                anchors.left: parent.left;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
        }

        Item {
            id: injectorGainContainer
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: injectorGainSliderTextMetrics.height
            anchors.top: avatarGainContainer.bottom;
            anchors.topMargin: 10;

            HifiControlsUit.Slider {
                id: injectorGainSlider
                anchors.right: parent.right
                height: parent.height
                width: 200
                minimumValue: -60.0
                maximumValue: 20.0
                stepSize: 5
                value: AudioScriptingInterface.getInjectorGain()
                onValueChanged: {
                    updateInjectorGainFromQML(value, false);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateInjectorGainFromQML(value, false);
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: {
                        // Do nothing.
                    }
                    onDoubleClicked: {
                        injectorGainSlider.value = 0.0
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
                id: injectorGainSliderTextMetrics
                text: injectorGainSliderText.text
                font: injectorGainSliderText.font
            }
            RalewayRegular {
                id: injectorGainSliderText;
                text: "Environment volume";
                size: 16;
                anchors.left: parent.left;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
        }

        Item {
            id: systemInjectorGainContainer
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: systemInjectorGainSliderTextMetrics.height
            anchors.top: injectorGainContainer.bottom;
            anchors.topMargin: 10;

            HifiControlsUit.Slider {
                id: systemInjectorGainSlider
                anchors.right: parent.right
                height: parent.height
                width: 200
                minimumValue: -60.0
                maximumValue: 20.0
                stepSize: 5
                value: AudioScriptingInterface.getSystemInjectorGain()
                onValueChanged: {
                    updateSystemInjectorGainFromQML(value, false);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateSystemInjectorGainFromQML(value, false);
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: {
                        // Do nothing.
                    }
                    onDoubleClicked: {
                        systemInjectorGainSlider.value = 0.0
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
                id: systemInjectorGainSliderTextMetrics
                text: systemInjectorGainSliderText.text
                font: systemInjectorGainSliderText.font
            }
            RalewayRegular {
                id: systemInjectorGainSliderText;
                text: "UI FX volume";
                size: 16;
                anchors.left: parent.left;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
        }

        Separator {
            id: secondSeparator;
            anchors.top: systemInjectorGainContainer.bottom;
            anchors.topMargin: 10;
        }
        
        Item {
            id: noiseReductionHeader
            x: margins.paddings;
            width: parent.width - margins.paddings * 2;
            height: 36;
            anchors.top: secondSeparator.bottom;
            anchors.topMargin: 10;

            HiFiGlyphs {
                width: margins.sizeCheckBox;
                text: hifi.glyphs.mic;
                color: hifi.colors.white;
                anchors.left: parent.left;
                anchors.leftMargin: -size / 4; // The glyph has empty space at left about 25%
                anchors.verticalCenter: parent.verticalCenter;
                size: 30;
            }

            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter;
                width: margins.sizeText + margins.sizeLevel;
                anchors.left: parent.left;
                anchors.leftMargin: margins.sizeCheckBox;
                size: 22;
                color: hifi.colors.white;
                text: qsTr("Noise Reduction");
            }
        }
        
        Item {
            id: noiseReductionSwitches;
            x: 2 * margins.paddings;
            width: parent.width;
            // switch heights + 2 * top margins
            height: (root.switchHeight) * 5 + 48;
            anchors.top: noiseReductionHeader.bottom;
            anchors.topMargin: 0;
        
            Item {
                 id: noiseReductionSwitchContainer;
                 x: margins.paddings;
                 width: parent.width / 2;
                 height: parent.height;
                 anchors.top: parent.top;
                 anchors.left: parent.left;
            
                HifiControlsUit.Switch {
                    id: acousticEchoCancellationSwitch;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: noiseReductionSwitchContainer.top
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: "Echo Cancellation";
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.acousticEchoCancellation;
                    onCheckedChanged: {
                        AudioScriptingInterface.acousticEchoCancellation = checked;
                        checked = Qt.binding(function () { return AudioScriptingInterface.acousticEchoCancellation; });
                    }
                }
                
                HifiControlsUit.Switch {
                    id: noiseReductionSwitch;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: acousticEchoCancellationSwitch.bottom;
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    labelTextOn: "Noise Reduction";
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: AudioScriptingInterface.noiseReduction;
                    onCheckedChanged: {
                        AudioScriptingInterface.noiseReduction = checked;
                        checked = Qt.binding(function () { return AudioScriptingInterface.noiseReduction; }); // restore binding
                    }
                }
                
                HifiControlsUit.Switch {
                    id: noiseReductionAutomaticSwitch;
                    height: root.switchHeight;
                    switchWidth: root.switchWidth;
                    anchors.top: noiseReductionSwitch.bottom;
                    anchors.topMargin: 24;
                    anchors.left: parent.left;
                    labelTextOn: "Manual Noise Reduction";
                    labelTextSize: 16;
                    backgroundOnColor: "#E3E3E3";
                    checked: !AudioScriptingInterface.noiseReductionAutomatic;
                    visible: AudioScriptingInterface.noiseReduction;
                    onCheckedChanged: {
                        AudioScriptingInterface.noiseReductionAutomatic = !checked;
                        checked = Qt.binding(function () { return !AudioScriptingInterface.noiseReductionAutomatic; }); // restore binding
                    }
                }
            }
        }
        
        Item {
            id: noiseReductionThresholdContainer
            x: margins.paddings;
            anchors.top: noiseReductionSwitches.bottom;
            anchors.topMargin: 16;
            width: parent.width - margins.paddings * 2;
            height: avatarGainSliderTextMetrics.height + 10;
            visible: AudioScriptingInterface.noiseReduction && !AudioScriptingInterface.noiseReductionAutomatic;

            HifiControlsUit.Slider {
                id: noiseReductionThresholdSlider
                anchors.right: parent.right
                height: noiseReductionThresholdSliderTextMetrics.height
                width: 200
                minimumValue: 0.0
                maximumValue: 1.0
                stepSize: 0.05
                value: AudioScriptingInterface.getNoiseReductionThreshold()
                onValueChanged: {
                    updateNoiseReductionThresholdFromQML(value);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateNoiseReductionThresholdFromQML(value);
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: {
                        // Do nothing.
                    }
                    onDoubleClicked: {
                        noiseReductionThresholdSlider.value = 0.0;
                    }
                    onPressed: {
                        // Pass through to Slider
                        mouse.accepted = false;
                    }
                    onReleased: {
                        // the above mouse.accepted seems to make this
                        // never get called, nonetheless...
                        mouse.accepted = false;
                    }
                }
            }
            TextMetrics {
                id: noiseReductionThresholdSliderTextMetrics
                text: noiseReductionThresholdSliderText.text
                font: noiseReductionThresholdSliderText.font
            }
            RalewayRegular {
                // The slider for my card is special, it controls the master gain
                id: noiseReductionThresholdSliderText;
                text: "Audio input threshold";
                size: 16;
                anchors.left: parent.left;
                color: hifi.colors.white;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
            
            Item {
                id: noisePeak
                anchors.right: parent.right
                anchors.rightMargin: 5;
                anchors.top: noiseReductionThresholdSlider.bottom;

                width: noiseReductionThresholdSlider.width - 10;
                height: 8;

                Text {
                    id: status;

                    anchors {
                        horizontalCenter: parent.horizontalCenter;
                        verticalCenter: parent.verticalCenter;
                    }
            
                    visible: AudioScriptingInterface.muted;
                    color: "#E2334D";
            
                    text: "MUTED";
                    font.pointSize: 10;
                }
            
                Item {
                    id: noiseBar;
                    
                    property bool gated: false;
                    property var level: AudioScriptingInterface.inputLevel;

                    width: parent.width;
                    height: parent.height;

                    anchors { fill: parent }

                    visible: !status.visible;

                    function onNoiseGateOpened() {
                        noiseBar.gated = false;
                    }

                    function onNoiseGateClosed() {
                        noiseBar.gated = true;
                    }

                    function onInputLevelChanged(level) {
                        noiseBar.level = level;
                    }

                    Component.onCompleted: {
                        AudioScriptingInterface.noiseGateOpened.connect(onNoiseGateOpened);
                        AudioScriptingInterface.noiseGateClosed.connect(onNoiseGateClosed);
                        AudioScriptingInterface.inputLevelChanged.connect(onInputLevelChanged);
                    }

                    Component.onDestruction: {
                        AudioScriptingInterface.noiseGateOpened.disconnect(onNoiseGateOpened);
                        AudioScriptingInterface.noiseGateClosed.disconnect(onNoiseGateClosed);
                        AudioScriptingInterface.inputLevelChanged.disconnect(onInputLevelChanged);
                    }

                    Rectangle { // base
                        radius: 4;
                        anchors { fill: parent }
                        color: colors.gutter;
                    }

                    Rectangle { // noiseMask
                        id: noiseMask;
                        width: parent.width * noiseBar.level;
                        radius: 5;
                        anchors {
                            bottom: parent.bottom;
                            bottomMargin: 0;
                            top: parent.top;
                            topMargin: 0;
                            left: parent.left;
                            leftMargin: 0;
                        }
                    }

                    LinearGradient {
                        anchors { fill: noiseMask }
                        source: noiseMask
                        start: Qt.point(0, 0);
                        end: Qt.point(noiseBar.width, 0);
                        gradient: Gradient {
                            GradientStop {
                                position: 0;
                                color: noiseBar.gated ? "#E2334D" : "#39A38F";
                            }
                            GradientStop {
                                position: 1;
                                color: noiseBar.gated ? "#E2334D" : "#39A38F";
                            }
                        }
                    }
                }
            }
        }
        
        Separator {
            id: thirdSeparator;
            anchors.top: noiseReductionThresholdContainer.bottom;
            anchors.topMargin: 14;
        }

        Item {
            id: inputDeviceHeader
            x: margins.paddings;
            width: parent.width - margins.paddings*2;
            height: 36;
            anchors.top: thirdSeparator.bottom;
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
                size: 22;
                color: hifi.colors.white;
                text: qsTr("Choose input device");
            }
        }

        AudioControls.LoopbackAudio {
            id: loopbackAudio
            x: margins.paddings
            anchors.top: inputDeviceHeader.bottom;
            anchors.topMargin: 10;
            visible: (bar.currentIndex === 1 && isVR) ||
                (bar.currentIndex === 0 && !isVR);
            anchors { left: parent.left; leftMargin: margins.paddings }
        }

        ListView {
            id: inputView;
            width: rightMostInputLevelPos;
            anchors.top: loopbackAudio.visible ? loopbackAudio.bottom : inputDeviceHeader.bottom;
            anchors.topMargin: 10;
            x: margins.paddings
            interactive: false;
            height: contentHeight;

            clip: true;
            model: AudioScriptingInterface.devices.input;
            delegate: Item {
                width: rightMostInputLevelPos - margins.paddings*2
                height: ((type != "hmd" && bar.currentIndex === 0) || (type != "desktop" && bar.currentIndex === 1)) ?
                        (margins.sizeCheckBox > checkBoxInput.implicitHeight ? margins.sizeCheckBox + 4 : checkBoxInput.implicitHeight + 4) : 0
                visible: (type != "hmd" && bar.currentIndex === 0) || (type != "desktop" && bar.currentIndex === 1)
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
                    fontSize: 16;
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

        Separator {
            id: fourthSeparator;
            anchors.top: inputView.bottom;
            anchors.topMargin: 10;
        }

        Item {
            id: outputDeviceHeader;
            anchors.topMargin: 10;
            anchors.top: fourthSeparator.bottom;
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
                size: 22;
                color: hifi.colors.white;
                text: qsTr("Choose output device");
            }
        }

        AudioControls.PlaySampleSound {
            id: playSampleSound
            x: margins.paddings
            anchors.top: outputDeviceHeader.bottom;
            anchors.topMargin: 10;
        }

        ListView {
            id: outputView
            width: parent.width - margins.paddings*2
            x: margins.paddings;
            interactive: false;
            height: contentHeight + 10;
            anchors.top: playSampleSound.bottom;
            anchors.topMargin: 10;
            clip: true;
            model: AudioScriptingInterface.devices.output;
            delegate: Item {
                width: rightMostInputLevelPos
                height: ((type != "hmd" && bar.currentIndex === 0) || (type != "desktop" && bar.currentIndex === 1)) ?
                        (margins.sizeCheckBox > checkBoxOutput.implicitHeight ? margins.sizeCheckBox + 4 : checkBoxOutput.implicitHeight + 4) : 0
                visible: (type != "hmd" && bar.currentIndex === 0) || (type != "desktop" && bar.currentIndex === 1)
                AudioControls.CheckBox {
                    id: checkBoxOutput
                    width: parent.width
                    spacing: margins.sizeCheckBox - boxSize
                    boxSize: margins.sizeCheckBox / 2
                    isRound: true
                    checked: bar.currentIndex === 0 ? selectedDesktop :  selectedHMD;
                    checkable: !checked
                    text: devicename
                    fontSize: 16
                    onPressed: {
                        if (!checked) {
                            AudioScriptingInterface.setOutputDevice(info, bar.currentIndex === 1);
                        }
                    }
                }
            }
        }
    }
}
