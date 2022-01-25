//
//  VR.qml
//
//  Created by Zach Fox on 2019-05-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../../simplifiedConstants" as SimplifiedConstants
import "../../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import QtQuick.Layouts 1.3

Flickable {
    id: root
    contentWidth: parent.width
    contentHeight: vrColumnLayout.height
    clip: true

    function changePeakValuesEnabled(enabled) {
        if (!enabled) {
            AudioScriptingInterface.devices.input.peakValuesEnabled = true;
        }
    }

    onVisibleChanged: {
        AudioScriptingInterface.devices.input.peakValuesEnabled = visible;
        if (visible) {
            root.contentX = 0;
            root.contentY = 0;
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.connect(changePeakValuesEnabled);
        } else {
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.disconnect(changePeakValuesEnabled);
        }
    }


    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Image {
        id: accent
        source: "../images/accent3.svg"
        anchors.left: parent.left
        anchors.top: parent.top
        width: 83
        height: 156
        transform: Scale {
            xScale: -1
            origin.x: accent.width / 2
            origin.y: accent.height / 2
        }
    }


    ColumnLayout {
        id: vrColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings

        ColumnLayout {
            id: controlsContainer
            Layout.preferredWidth: parent.width
            Layout.topMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: controlsTitle
                text: "VR Movement Controls"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                id: controlsRadioButtonGroup
                width: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons

                ButtonGroup { id: controlsButtonGroup }

                SimplifiedControls.RadioButton {
                    id: controlsDefault
                    text: "Default"
                    ButtonGroup.group: controlsButtonGroup
                    checked: MyAvatar.getControlScheme() === 0
                    onClicked: {
                        MyAvatar.setControlScheme(0);
                    }
                }

                SimplifiedControls.RadioButton {
                    id: controlsAnalog
                    text: "Analog"
                    ButtonGroup.group: controlsButtonGroup
                    checked: MyAvatar.getControlScheme() === 1
                    onClicked: {
                        MyAvatar.setControlScheme(1);
                    }
                }

                Item {
                    id: controlsAdvancedContainer
                    Layout.minimumWidth: parent.width
                    Layout.minimumHeight: 14

                    SimplifiedControls.RadioButton {
                        id: controlsAdvanced
                        text: "Advanced"
                        ButtonGroup.group: controlsButtonGroup
                        checked: MyAvatar.getControlScheme() === 2
                        onClicked: {
                            MyAvatar.setControlScheme(2);
                        }
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        height: 14
                    }

                    SimplifiedControls.Slider {
                        id: controlsAdvancedMovementSpeed
                        anchors.top: parent.top
                        anchors.topMargin: 4 // For perfect alignment
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        width: 300
                        height: 14
                        labelText: "Movement Speed"
                        labelTextColor: simplifiedUI.colors.text.darkGrey
                        from: 3
                        to: 30
                        defaultValue: 6
                        value: MyAvatar.analogPlusWalkSpeed
                        live: true
                        onValueChanged: {
                            if (MyAvatar.analogPlusWalkSpeed != controlsAdvancedMovementSpeed.value) {
                                MyAvatar.analogPlusWalkSpeed = controlsAdvancedMovementSpeed.value;
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                text: "VR Rotation Mode"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                width: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons

                ButtonGroup { id: rotationButtonGroup }

                SimplifiedControls.RadioButton {
                    text: "Snap Turn"
                    ButtonGroup.group: rotationButtonGroup
                    checked: MyAvatar.getSnapTurn() === true
                    onClicked: {
                        MyAvatar.setSnapTurn(true);
                    }
                }

                SimplifiedControls.RadioButton {
                    text: "Smooth Turn"
                    ButtonGroup.group: rotationButtonGroup
                    checked: MyAvatar.getSnapTurn() === false
                    onClicked: {
                        MyAvatar.setSnapTurn(false);
                    }
                }
            }
        }
        
        ColumnLayout {
            id: micControlsContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: micControlsTitle
                text: "Default Mute Controls"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                id: micControlsSwitchGroup
                Layout.preferredWidth: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

                SimplifiedControls.Switch {
                    id: muteMicrophoneSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Mute Microphone"
                    checked: AudioScriptingInterface.mutedHMD
                    onClicked: {
                        AudioScriptingInterface.mutedHMD = !AudioScriptingInterface.mutedHMD;
                    }
                }

                SimplifiedControls.Switch {
                    id: pushToTalkSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Push to Talk - Press and Hold Grip Triggers to Talk"
                    checked: AudioScriptingInterface.pushToTalkHMD
                    onClicked: {
                        AudioScriptingInterface.pushToTalkHMD = !AudioScriptingInterface.pushToTalkHMD;
                    }
                }
            }
        }

        ColumnLayout {
            id: inputDeviceContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: inputDeviceTitle
                text: "Which input device?"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ButtonGroup { id: inputDeviceButtonGroup }

            ListView {
                id: inputDeviceListView
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: contentItem.height
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false
                clip: true
                model: AudioScriptingInterface.devices.input
                delegate: Item {
                    width:   parent.width
                    height:  model.type != "desktop" ? inputDeviceCheckbox.height + simplifiedUI.margins.settings.spacingBetweenRadiobuttons : 0
                    visible: model.type != "desktop"
                     
                    SimplifiedControls.RadioButton {
                        id: inputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width - inputLevel.width
                        height: 16
                        checked: selectedHMD
                        text: model.devicename
                        wrapLabel: false
                        ButtonGroup.group: inputDeviceButtonGroup
                        onClicked: {
                            AudioScriptingInterface.setStereoInput(false); // the next selected audio device might not support stereo
                            AudioScriptingInterface.setInputDevice(model.info, true); // `true` argument for HMD mode setting
                        }
                    }

                    SimplifiedControls.InputPeak {
                        id: inputLevel
                        showMuted: AudioScriptingInterface.mutedHMD
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        peak: model.peak
                        visible: AudioScriptingInterface.devices.input.peakValuesAvailable
                    }
                }
            }

            SimplifiedControls.Button {
                id: audioLoopbackButton
                property bool audioLoopedBack: AudioScriptingInterface.getLocalEcho()
                
                function startAudioLoopback() {
                    if (!audioLoopedBack) {
                        audioLoopedBack = true;
                        AudioScriptingInterface.setLocalEcho(true);
                    }
                }
                function stopAudioLoopback() {
                    if (audioLoopedBack) {
                        audioLoopedBack = false;
                        AudioScriptingInterface.setLocalEcho(false);
                    }
                }

                Component.onDestruction: stopAudioLoopback();

                onVisibleChanged: {
                    if (!visible) {
                        stopAudioLoopback();
                    }
                }

                enabled: HMD.active
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                width: 160
                height: 32
                text: audioLoopedBack ? "STOP TESTING" : "TEST YOUR MIC"
                onClicked: {
                    if (audioLoopedBack) {
                        stopAudioLoopback();
                    } else {
                        startAudioLoopback();
                    }
                }
            }
        }

        ColumnLayout {
            id: outputDeviceContainer
            Layout.preferredWidth: parent.width
            Layout.bottomMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: outputDeviceTitle
                text: "Which output device?"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ButtonGroup { id: outputDeviceButtonGroup }

            ListView {
                id: outputDeviceListView
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: contentItem.height
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false
                clip: true
                model: AudioScriptingInterface.devices.output
                delegate: Item {
                    width: parent.width
                    height:  model.type != "desktop" ? outputDeviceCheckbox.height + simplifiedUI.margins.settings.spacingBetweenRadiobuttons : 0
                    visible: model.type != "desktop"
                    SimplifiedControls.RadioButton {
                        id: outputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width
                        height: 16
                        checked: selectedHMD
                        text: model.devicename
                        wrapLabel: false
                        ButtonGroup.group: outputDeviceButtonGroup
                        onClicked: {
                            AudioScriptingInterface.setOutputDevice(model.info, true); // `true` argument for VR mode setting
                        }
                    }
                }
            }

            SimplifiedControls.Button {
                property var sound: null
                property var sample: null
                property bool isPlaying: false
                function createSampleSound() {
                    sound = ApplicationInterface.getSampleSound();
                    sample = null;
                }
                function playSound() {
                    if (sample === null && !isPlaying) {
                        sample = AudioScriptingInterface.playSystemSound(sound);
                        isPlaying = true;
                        sample.finished.connect(reset);
                    }
                }
                function stopSound() {
                    if (sample && isPlaying) {
                        sample.stop();
                    }
                }

                function reset() {
                    sample.finished.disconnect(reset);
                    isPlaying = false;
                    sample = null;
                }

                Component.onCompleted: createSampleSound();
                Component.onDestruction: stopSound();

                onVisibleChanged: {
                    if (!visible) {
                        stopSound();
                    }
                }

                id: testYourSoundButton
                enabled: HMD.active
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                width: 160
                height: 32
                text: isPlaying ? "STOP TESTING" : "TEST YOUR SOUND"
                onClicked: {
                    isPlaying ? stopSound() : playSound()
                }
            }
        }
    }
}
