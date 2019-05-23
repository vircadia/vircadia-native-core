//
//  Audio.qml
//
//  Created by Zach Fox on 2019-05-06
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
    contentHeight: audioColumnLayout.height
    topMargin: 24
    bottomMargin: 24
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
            root.contentY = -root.topMargin;
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.connect(changePeakValuesEnabled);
        } else {
            AudioScriptingInterface.devices.input.peakValuesEnabledChanged.disconnect(changePeakValuesEnabled);
        }
    }


    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    ColumnLayout {
        id: audioColumnLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings
        
        ColumnLayout {
            id: volumeControlsContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: volumeControlsTitle
                text: "Volume Controls"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            SimplifiedControls.Slider {
                id: peopleVolume
                Layout.preferredWidth: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                height: 30
                labelText: "People Volume"
                from: simplifiedUI.numericConstants.mutedValue
                to: 20.0
                defaultValue: 0.0
                stepSize: 5.0
                value: AudioScriptingInterface.avatarGain
                live: true
                function updatePeopleGain(sliderValue) {
                    if (AudioScriptingInterface.avatarGain !== sliderValue) {
                        AudioScriptingInterface.avatarGain = sliderValue;
                    }
                }
                onValueChanged: {
                    updatePeopleGain(value);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updatePeopleGain(value);
                    }
                }
            }

            SimplifiedControls.Slider {
                id: environmentVolume
                Layout.preferredWidth: parent.width
                Layout.topMargin: 2
                height: 30
                labelText: "Environment Volume"
                from: simplifiedUI.numericConstants.mutedValue
                to: 20.0
                defaultValue: 0.0
                stepSize: 5.0
                value: AudioScriptingInterface.serverInjectorGain
                live: true
                function updateEnvironmentGain(sliderValue) {
                    if (AudioScriptingInterface.serverInjectorGain !== sliderValue) {
                        AudioScriptingInterface.serverInjectorGain = sliderValue;
                        AudioScriptingInterface.localInjectorGain = sliderValue;
                    }
                }
                onValueChanged: {
                    updateEnvironmentGain(value);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateEnvironmentGain(value);
                    }
                }
            }

            SimplifiedControls.Slider {
                id: systemSoundVolume
                Layout.preferredWidth: parent.width
                Layout.topMargin: 2
                height: 30
                labelText: "System Sound Volume"
                from: simplifiedUI.numericConstants.mutedValue
                to: 20.0
                defaultValue: 0.0
                stepSize: 5.0
                value: AudioScriptingInterface.systemInjectorGain
                live: true
                function updateSystemGain(sliderValue) {
                    if (AudioScriptingInterface.systemInjectorGain !== sliderValue) {
                        AudioScriptingInterface.systemInjectorGain = sliderValue;
                    }
                }
                onValueChanged: {
                    updateSystemGain(value);
                }
                onPressedChanged: {
                    if (!pressed) {
                        updateSystemGain(value);
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

            ColumnLayout {
                id: micControlsSwitchGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

                SimplifiedControls.Switch {
                    id: muteMicrophoneSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Mute Microphone"
                    checked: AudioScriptingInterface.mutedDesktop
                    onClicked: {
                        AudioScriptingInterface.mutedDesktop = !AudioScriptingInterface.mutedDesktop;
                    }
                }

                SimplifiedControls.Switch {
                    id: pushToTalkSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Push to Talk - Press and Hold \"T\" to Talk"
                    checked: AudioScriptingInterface.pushToTalkDesktop
                    onClicked: {
                        AudioScriptingInterface.pushToTalkDesktop = !AudioScriptingInterface.pushToTalkDesktop;
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

            ButtonGroup { id: inputDeviceButtonGroup }

            ListView {
                id: inputDeviceListView
                Layout.preferredWidth: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false
                height: contentItem.height
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons
                clip: true
                model: AudioScriptingInterface.devices.input
                delegate: Item {
                    width: parent.width
                    height: inputDeviceCheckbox.height

                    SimplifiedControls.RadioButton {
                        id: inputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width - inputLevel.width
                        height: 16
                        wrapLabel: false
                        checked: selectedDesktop
                        text: model.devicename
                        ButtonGroup.group: inputDeviceButtonGroup
                        onClicked: {
                            AudioScriptingInterface.setStereoInput(false); // the next selected audio device might not support stereo
                            AudioScriptingInterface.setInputDevice(model.info, false); // `false` argument for Desktop mode setting
                        }
                    }

                    SimplifiedControls.InputPeak {
                        id: inputLevel
                        showMuted: AudioScriptingInterface.mutedDesktop
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        peak: model.peak
                        visible: AudioScriptingInterface.devices.input.peakValuesAvailable
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
                    interval: 8000
                    running: false
                    repeat: false
                    onTriggered: {
                        stopAudioLoopback();
                    }
                }

                id: testYourMicButton
                enabled: !HMD.active
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

            ButtonGroup { id: outputDeviceButtonGroup }

            ListView {
                id: outputDeviceListView
                Layout.preferredWidth: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                interactive: false
                height: contentItem.height
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons
                clip: true
                model: AudioScriptingInterface.devices.output
                delegate: Item {
                    width: parent.width
                    height: outputDeviceCheckbox.height

                    SimplifiedControls.RadioButton {
                        id: outputDeviceCheckbox
                        anchors.left: parent.left
                        width: parent.width
                        height: 16
                        checked: selectedDesktop
                        text: model.devicename
                        wrapLabel: false
                        ButtonGroup.group: outputDeviceButtonGroup
                        onClicked: {
                            AudioScriptingInterface.setOutputDevice(model.info, false); // `false` argument for Desktop mode setting
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
                enabled: !HMD.active
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
