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

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"
import "./" as AudioControls

Rectangle {
    id: root;

    HifiConstants { id: hifi; }

    property var eventBridge;
    property string title: "Audio Settings - " + Audio.context;
    signal sendToScript(var message);

    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"
    function showTitle() {
        return (root.parent !== null) && root.parent.objectName == "loader";
    }

    property bool isVR: Audio.context === "VR"
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
        height: 36

        AudioControls.AudioTabButton {
            height: parent.height
            text: qsTr("Desktop")
        }
        AudioControls.AudioTabButton {
            height: parent.height
            text: qsTr("VR")
        }
    }

    Column {
        spacing: 12;
        anchors.topMargin: 8
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        width: parent.width;

        RalewayRegular {
            x: margins.paddings;
            size: 16;
            color: "white";
            text: qsTr("Mic Settings")
        }

        Separator { }

        ColumnLayout {
            x: margins.paddings; // padding does not work
            spacing: 16;
            width: parent.width;

            // mute is in its own row
            RowLayout {
                AudioControls.CheckBox {
                    text: qsTr("Mute microphone");
                    isRedCheck: true;
                    checked: Audio.muted;
                    onClicked: {
                        Audio.muted = checked;
                        checked = Qt.binding(function() { return Audio.muted; }); // restore binding
                    }
                }
            }

            RowLayout {
                spacing: 16;
                AudioControls.CheckBox {
                    text: qsTr("Enable noise reduction");
                    checked: Audio.noiseReduction;
                    onClicked: {
                        Audio.noiseReduction = checked;
                        checked = Qt.binding(function() { return Audio.noiseReduction; }); // restore binding
                    }
                }
                AudioControls.CheckBox {
                    text: qsTr("Show audio level meter");
                    checked: AvatarInputs.showAudioTools;
                    onClicked: {
                        AvatarInputs.showAudioTools = checked;
                        checked = Qt.binding(function() { return AvatarInputs.showAudioTools; }); // restore binding
                    }
                }
            }
        }

        Separator {}

        RowLayout {
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: 28
            spacing: 0
            HiFiGlyphs {
                Layout.minimumWidth: margins.sizeCheckBox
                Layout.maximumWidth: margins.sizeCheckBox
                text: hifi.glyphs.mic;
                color: hifi.colors.primaryHighlight;
                anchors.verticalCenter: parent.verticalCenter;
                size: 36;
            }
            RalewayRegular {
                Layout.minimumWidth: margins.sizeText + margins.sizeLevel
                Layout.maximumWidth: margins.sizeText + margins.sizeLevel
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("CHOOSE INPUT DEVICE");
            }
        }

        ListView {
            id: inputView
            width: parent.width - margins.paddings*2
            x: margins.paddings
            height: 145;
            spacing: 4;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.input;
            delegate: RowLayout {
                width: inputView.width;
                spacing: 5

                AudioControls.CheckBox {
                    Layout.fillWidth: true
                    checked: bar.currentIndex === 0 ? selectedDesktop :  selectedHMD;
                    boxRadius: boxSize/2
                    text: devicename
                    onClicked: {
                        if (checked) {
                            Audio.setInputDevice(info, bar.currentIndex === 1);
                        }
                    }
                }
                InputLevel {
                    visible: (bar.currentIndex === 1 && selectedHMD && isVR) ||
                             (bar.currentIndex === 0 && selectedDesktop && !isVR);
                }
            }
        }

        Separator {}

        RowLayout {
            x: margins.paddings;
            width: parent.width - margins.paddings*2
            height: 28
            spacing: 0
            HiFiGlyphs {
                Layout.minimumWidth: margins.sizeCheckBox
                Layout.maximumWidth: margins.sizeCheckBox
                text: hifi.glyphs.unmuted;
                color: hifi.colors.primaryHighlight;
                anchors.verticalCenter: parent.verticalCenter;
                size: 28;
            }
            RalewayRegular {
                Layout.minimumWidth: margins.sizeText + margins.sizeLevel
                Layout.maximumWidth: margins.sizeText + margins.sizeLevel
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
            height: Math.min(220, contentHeight);
            spacing: 4;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.output;
            delegate: RowLayout {
                width: outputView.width;
                spacing: 0

                AudioControls.CheckBox {
                    Layout.fillWidth: true
                    boxRadius: boxSize/2
                    checked: bar.currentIndex === 0 ? selectedDesktop :  selectedHMD;
                    text: devicename
                    onClicked: {
                        if (checked) {
                            Audio.setOutputDevice(info, bar.currentIndex === 1);
                        }
                    }
                }
            }
        }
        PlaySampleSound { anchors { left: parent.left; leftMargin: margins.paddings }}
    }
}
