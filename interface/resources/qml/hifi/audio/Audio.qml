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

    Column {
        y: 16; // padding does not work
        spacing: 16;
        width: parent.width;

        RalewayRegular {
            x: margins.paddings; // padding does not work
            size: 16;
            color: "white";
            text: root.title;

            visible: root.showTitle();
        }

        Separator { visible: root.showTitle() }

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

            RalewayRegular {
                Layout.minimumWidth: margins.sizeDesktop
                Layout.maximumWidth: margins.sizeDesktop
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("DESKTOP");
            }

            RalewayRegular {
                Layout.minimumWidth: margins.sizeVR
                Layout.maximumWidth: margins.sizeVR
                Layout.alignment: Qt.AlignRight
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("VR");
            }
        }

        ListView {
            id: inputView
            width: parent.width - margins.paddings*2
            x: margins.paddings
            height: 125;
            spacing: 0;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.input;
            delegate: RowLayout {
                width: inputView.width;
                height: 36;
                spacing: 0

                RalewaySemiBold {
                    Layout.minimumWidth: margins.sizeCheckBox + margins.sizeText
                    Layout.maximumWidth: margins.sizeCheckBox + margins.sizeText
                    clip: true
                    size: 16;
                    color: "white";
                    text: devicename;
                }

                //placeholder for invisible level
                Item {
                    Layout.minimumWidth: margins.sizeLevel
                    Layout.maximumWidth: margins.sizeLevel
                    height: 8;
                    InputLevel {
                        visible: (isVR && selectedHMD) || (!isVR && selectedDesktop);
                        anchors.fill: parent
                    }
                }
                AudioControls.CheckBox {
                    Layout.minimumWidth: margins.sizeDesktop
                    Layout.maximumWidth: margins.sizeDesktop
                    leftPadding: margins.sizeDesktop/2 - boxSize/2
                    checked: selectedDesktop;
                    onClicked: {
                        if (checked) {
                            Audio.setInputDevice(info, false);
                        }
                    }
                }

                AudioControls.CheckBox {
                    Layout.minimumWidth: margins.sizeVR
                    Layout.maximumWidth: margins.sizeVR
                    leftPadding: margins.sizeVR/2 - boxSize/2
                    checked: selectedHMD;
                    onClicked: {
                        if (checked) {
                            Audio.setInputDevice(info, true);
                        }
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

            RalewayRegular {
                Layout.minimumWidth: margins.sizeDesktop
                Layout.maximumWidth: margins.sizeDesktop
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("DESKTOP");
            }

            RalewayRegular {
                Layout.minimumWidth: margins.sizeVR
                Layout.maximumWidth: margins.sizeVR
                Layout.alignment: Qt.AlignRight
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("VR");
            }
        }

        ListView {
            id: outputView
            width: parent.width - margins.paddings*2
            x: margins.paddings
            height: Math.min(220, contentHeight);
            spacing: 0;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.output;
            delegate: RowLayout {
                width: outputView.width;
                height: 36;
                spacing: 0

                RalewaySemiBold {
                    Layout.minimumWidth: margins.sizeCheckBox + margins.sizeText
                    Layout.maximumWidth: margins.sizeCheckBox + margins.sizeText
                    clip: true
                    size: 16;
                    color: "white";
                    text: devicename;
                }

                //placeholder for invisible level
                Item {
                    Layout.minimumWidth: margins.sizeLevel
                    Layout.maximumWidth: margins.sizeLevel
                    height: 8;
                }
                AudioControls.CheckBox {
                    Layout.minimumWidth: margins.sizeDesktop
                    Layout.maximumWidth: margins.sizeDesktop
                    leftPadding: margins.sizeDesktop/2 - boxSize/2
                    checked: selectedDesktop;
                    onClicked: {
                        if (checked) {
                            Audio.setOutputDevice(info, false);
                        }
                    }
                }
                AudioControls.CheckBox {
                    Layout.minimumWidth: margins.sizeVR
                    Layout.maximumWidth: margins.sizeVR
                    leftPadding: margins.sizeVR/2 - boxSize/2
                    checked: selectedHMD;
                    onClicked: {
                        if (checked) {
                            Audio.setOutputDevice(info, true);
                        }
                    }
                }
            }
        }
        PlaySampleSound { anchors { left: parent.left; leftMargin: margins.paddings }}
    }
}
