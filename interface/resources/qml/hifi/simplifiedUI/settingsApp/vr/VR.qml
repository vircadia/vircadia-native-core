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
    id: root;
    contentWidth: parent.width;
    contentHeight: vrColumnLayout.height;
    topMargin: 16
    bottomMargin: 16
    clip: true;

    function changePeakValuesEnabled(enabled) {
        if (!enabled) {
            AudioScriptingInterface.devices.input.peakValuesEnabled = true;
        }
    }

    onVisibleChanged: {
        AudioScriptingInterface.devices.input.peakValuesEnabled = visible;
        if (visible) {
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.connect(changePeakValuesEnabled);
        } else {
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.disconnect(changePeakValuesEnabled);
        }
    }


    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    ColumnLayout {
        id: vrColumnLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings

        ColumnLayout {
            id: controlsContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: controlsTitle
                text: "VR Movement Controls"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                id: controlsSubtitle
                Layout.topMargin: simplifiedUI.margins.settings.subtitleTopMargin
                text: "Everyone responds to VR movement differently. Choose the setting that is most comfortable for you. Try using \"Default\" first."
                wrapMode: Text.Wrap
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 14
                color: simplifiedUI.colors.text.darkGrey
            }

            ColumnLayout {
                id: controlsRadioButtonGroup
                width: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

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
            id: micControlsContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: micControlsTitle
                text: "Default Mute Controls"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                id: micControlsSubtitle
                Layout.topMargin: simplifiedUI.margins.settings.subtitleTopMargin
                text: "These settings let you configure how you communicate with others in your HQ while in VR mode. Push to Talk works like a walkie-talkie!"
                wrapMode: Text.Wrap
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 14
                color: simplifiedUI.colors.text.darkGrey
            }

            ColumnLayout {
                id: micControlsSwitchGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

                SimplifiedControls.Switch {
                    id: muteMicrophoneSwitch
                    width: parent.width
                    height: simplifiedUI.sizes.controls.simplifiedSwitch.switchBackgroundHeight
                    labelTextOn: "Mute Microphone"
                    checked: AudioScriptingInterface.mutedHMD
                    onClicked: {
                        AudioScriptingInterface.mutedHMD = !AudioScriptingInterface.mutedHMD;
                    }
                }

                SimplifiedControls.Switch {
                    id: pushToTalkSwitch
                    width: parent.width
                    height: simplifiedUI.sizes.controls.simplifiedSwitch.switchBackgroundHeight
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

            HifiStylesUit.GraphikRegular {
                id: inputDeviceTitle
                text: "Which input device?"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                id: inputDeviceSubtitle
                Layout.topMargin: simplifiedUI.margins.settings.subtitleTopMargin
                text: "Here are all of the input devices and microphones that we found. Select the one you'd like to use in your HQ while in VR mode."
                wrapMode: Text.Wrap
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 14
                color: simplifiedUI.colors.text.darkGrey
            }

            ButtonGroup { id: inputDeviceButtonGroup }

            ListView {
                id: inputDeviceListView;
                anchors.left: parent.left
                anchors.right: parent.right
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false;
                height: contentItem.height;
                spacing: 4;
                clip: true;
                model: AudioScriptingInterface.devices.input;
                delegate: Item {
                    width: parent.width
                    height: inputDeviceCheckbox.height

                    SimplifiedControls.RadioButton {
                        id: inputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width - inputLevel.width
                        checked: selectedHMD;
                        text: model.devicename
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
                        peak: model.peak;
                        visible: AudioScriptingInterface.devices.input.peakValuesAvailable;
                    }
                }
            }

            SimplifiedControls.Button {
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

                Timer {
                    id: loopbackTimer
                    interval: 8000;
                    running: false;
                    repeat: false;
                    onTriggered: {
                        stopAudioLoopback();
                    }
                }

                id: testYourMicButton
                enabled: HMD.active
                anchors.left: parent.left
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                width: 160
                height: 32
                text: audioLoopedBack ? "STOP TESTING" : "TEST YOUR MIC"
                onClicked: {
                    if (audioLoopedBack) {
                        loopbackTimer.stop();
                        stopAudioLoopback();
                    } else {
                        loopbackTimer.restart();
                        startAudioLoopback();
                    }
                }
            }
        }

        ColumnLayout {
            id: outputDeviceContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: outputDeviceTitle
                text: "Which output device?"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                id: outputDeviceSubtitle
                Layout.topMargin: simplifiedUI.margins.settings.subtitleTopMargin
                text: "Here are all of the output devices that we found. Select the one you'd like to use in your HQ while in VR mode."
                wrapMode: Text.Wrap
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 14
                color: simplifiedUI.colors.text.darkGrey
            }

            ButtonGroup { id: outputDeviceButtonGroup }

            ListView {
                id: outputDeviceListView;
                anchors.left: parent.left
                anchors.right: parent.right
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false;
                height: contentItem.height;
                spacing: 4;
                clip: true;
                model: AudioScriptingInterface.devices.output;
                delegate: Item {
                    width: parent.width
                    height: outputDeviceCheckbox.height

                    SimplifiedControls.RadioButton {
                        id: outputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width
                        checked: selectedDesktop;
                        text: model.devicename
                        ButtonGroup.group: outputDeviceButtonGroup
                        onClicked: {
                            AudioScriptingInterface.setOutputDevice(model.info, true); // `false` argument for Desktop mode setting
                        }
                    }
                }
            }

            SimplifiedControls.Button {
                property var sound: null;
                property var sample: null;
                property bool isPlaying: false;
                function createSampleSound() {
                    sound = ApplicationInterface.getSampleSound();
                    sample = null;
                }
                function playSound() {
                    // FIXME: MyAvatar is not properly exposed to QML; MyAvatar.qmlPosition is a stopgap
                    // FIXME: AudioScriptingInterface.playSystemSound should not require position
                    if (sample === null && !isPlaying) {
                        sample = AudioScriptingInterface.playSystemSound(sound, MyAvatar.qmlPosition);
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
                anchors.left: parent.left
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                width: 160
                height: 32
                text: isPlaying ? "STOP TESTING" : "TEST YOUR SOUND"
                onClicked: {
                    isPlaying ? stopSound() : playSound();
                }
            }
        }
    }
}
