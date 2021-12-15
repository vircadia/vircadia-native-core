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
        source: "../images/accent2.svg"
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
        id: audioColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings
        
        ColumnLayout {
            id: volumeControlsContainer
            Layout.preferredWidth: parent.width
            Layout.topMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
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
                Layout.preferredHeight: 30
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
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
                Layout.preferredHeight: 30
                Layout.topMargin: 2
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
                Layout.preferredHeight: 30
                Layout.topMargin: 2
                labelText: "UI FX Volume"
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
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: parent.width
                    labelTextOn: "Mute Microphone"
                    checked: AudioScriptingInterface.mutedDesktop
                    onClicked: {
                        AudioScriptingInterface.mutedDesktop = !AudioScriptingInterface.mutedDesktop;
                    }
                }

                SimplifiedControls.Switch {
                    id: pushToTalkSwitch
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: parent.width
                    labelTextOn: "Push to Talk - Press and Hold \"T\" to Talk"
                    checked: AudioScriptingInterface.pushToTalkDesktop
                    onClicked: {
                        AudioScriptingInterface.pushToTalkDesktop = !AudioScriptingInterface.pushToTalkDesktop;
                    }
                }

                SimplifiedControls.Switch {
                    id: attenuateOutputSwitch
                    enabled: AudioScriptingInterface.pushToTalkDesktop
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: parent.width
                    labelTextOn: "Reduce volume of other sounds while I'm pushing-to-talk"
                    checked: AudioScriptingInterface.pushingToTalkOutputGainDesktop !== 0.0
                    onClicked: {
                        if (AudioScriptingInterface.pushingToTalkOutputGainDesktop === 0.0) {
                            AudioScriptingInterface.pushingToTalkOutputGainDesktop = -20.0;
                        } else {
                            AudioScriptingInterface.pushingToTalkOutputGainDesktop = 0.0;
                        }
                    }
                }

                SimplifiedControls.Switch {
                    id: acousticEchoCancellationSwitch
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: parent.width
                    labelTextOn: "Acoustic Echo Cancellation"
                    checked: AudioScriptingInterface.acousticEchoCancellation
                    onClicked: {
                        AudioScriptingInterface.acousticEchoCancellation = !AudioScriptingInterface.acousticEchoCancellation;
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
                     width:  parent.width
                     height: model.type != "hmd" ? inputDeviceCheckbox.height + simplifiedUI.margins.settings.spacingBetweenRadiobuttons : 0
                     visible: model.type != "hmd"
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

                enabled: !HMD.active
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
                    height: model.type != "hmd" ? outputDeviceCheckbox.height +simplifiedUI.margins.settings.spacingBetweenRadiobuttons : 0
                    visible: model.type != "hmd"
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
