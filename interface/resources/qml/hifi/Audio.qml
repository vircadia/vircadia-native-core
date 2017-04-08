//
//  Audio.qml
//  qml/hifi
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
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0

import "../styles-uit"
import "../controls-uit" as HifiControls

import "components"

Rectangle {
    id: audio;

    //put info text here
    property alias infoText: infoArea.text

    color: "#404040";

    HifiConstants { id: hifi; }
    objectName: "AudioWindow"

    property var eventBridge;
    property string title: "Audio Options"
    signal sendToScript(var message);

    //set models after Components is shown
    Component.onCompleted: {
        refreshTimer.start()
        refreshTimerOutput.start()
    }

    Component {
        id: separator
        LinearGradient {
            start: Qt.point(0, 0)
            end: Qt.point(0, 4)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#303030" }
                GradientStop { position: 0.33; color: "#252525" }  // Equivalent of darkGray0 over baseGray background.
                GradientStop { position: 0.5; color: "#303030" }
                GradientStop { position: 0.6; color: "#454a49" }
                GradientStop { position: 1.0; color: "#454a49" }
            }
            cached: true
        }
    }

    Column {
        anchors { left: parent.left; right: parent.right }
        spacing: 8

        RalewayRegular {
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            height: 45
            size: 20
            color: "white"
            text: audio.title
        }

        Loader {
            width: parent.width
            height: 5
            sourceComponent: separator
        }

        //connections required to syncronize with Menu
        Connections {
            target: AudioDevice
            onMuteToggled: {
                audioMute.checkbox.checked = AudioDevice.getMuted()
            }
        }

        Connections {
            target: AvatarInputs
            onShowAudioToolsChanged: {
                audioTools.checkbox.checked = showAudioTools
            }
        }

        AudioCheckbox {
            id: audioMute
            width: parent.width
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            checkbox.checked: AudioDevice.muted
            text.text: qsTr("Mute microphone")
            onCheckBoxClicked: {
                AudioDevice.muted = checked
            }
        }

        AudioCheckbox {
            id: audioTools
            width: parent.width
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            checkbox.checked: AvatarInputs.showAudioTools
            text.text: qsTr("Show audio level meter")
            onCheckBoxClicked: {
                AvatarInputs.showAudioTools = checked
            }
        }

        Loader {
            width: parent.width
            height: 5
            sourceComponent: separator
        }

        Row {
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            height: 40
            spacing: 8

            HiFiGlyphs {
                text: hifi.glyphs.mic
                color: hifi.colors.primaryHighlight
                anchors.verticalCenter: parent.verticalCenter
                size: 32
            }
            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter
                size: 16
                color: "#AFAFAF"
                text: qsTr("CHOOSE INPUT DEVICE")
            }
        }

        ListView {
            Timer {
                id: refreshTimer
                interval: 1
                repeat: false
                onTriggered: {
                    //refresh model
                    inputAudioListView.model = undefined
                    inputAudioListView.model = AudioDevice.inputAudioDevices
                }
            }
            id: inputAudioListView
            anchors { left: parent.left; right: parent.right; leftMargin: 70 }
            height: 125
            spacing: 16
            clip: true
            snapMode: ListView.SnapToItem
            delegate: AudioCheckbox {
                width: parent.width
                checkbox.checked: (modelData === AudioDevice.getInputDevice())
                text.text: modelData
                onCheckBoxClicked: {
                    if (checked) {
                        AudioDevice.setInputDevice(modelData)
                        refreshTimer.start()
                    }
                }
            }
        }

        Loader {
            width: parent.width
            height: 5
            sourceComponent: separator
        }

        Row {
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            height: 40
            spacing: 8

            HiFiGlyphs {
                text: hifi.glyphs.unmuted
                color: hifi.colors.primaryHighlight
                anchors.verticalCenter: parent.verticalCenter
                size: 32
            }
            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter
                size: 16
                color: "#AFAFAF"
                text: qsTr("CHOOSE OUTPUT DEVICE")
            }
        }
        ListView {
            id: outputAudioListView
            Timer {
                id: refreshTimerOutput
                interval: 1
                repeat: false
                onTriggered: {
                    //refresh model
                    outputAudioListView.model = undefined
                    outputAudioListView.model = AudioDevice.outputAudioDevices
                }
            }
            anchors { left: parent.left; right: parent.right; leftMargin: 70 }
            height: 250
            spacing: 16
            clip: true
            snapMode: ListView.SnapToItem
            delegate: AudioCheckbox {
                width: parent.width
                checkbox.checked: (modelData === AudioDevice.getOutputDevice())
                text.text: modelData
                onCheckBoxClicked: {
                    if (checked) {
                        AudioDevice.setOutputDevice(modelData)
                        refreshTimerOutput.start()
                    }
                }
            }
        }

        Loader {
            id: lastSeparator
            width: parent.width
            height: 6
            sourceComponent: separator
        }

        Row {
            anchors { left: parent.left; right: parent.right; leftMargin: 30 }
            height: 40
            spacing: 8

            HiFiGlyphs {
                id: infoSign
                text: hifi.glyphs.info
                color: "#AFAFAF"
                anchors.verticalCenter: parent.verticalCenter
                size: 60
            }
            RalewayRegular {
                id: infoArea
                width: parent.width - infoSign.implicitWidth - parent.spacing - 10
                wrapMode: Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
                size: 12
                color: hifi.colors.baseGrayHighlight
            }
        }
    }
}
