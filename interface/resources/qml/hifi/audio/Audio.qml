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
import QtQuick.Controls 1.4
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
        return root.parent.objectName == "loader";
    }

    property bool showPeaks: true;
    function enablePeakValues() {
        Audio.devices.input.peakValuesEnabled = true;
        Audio.devices.input.peakValuesEnabledChanged.connect(function(enabled) {
            if (!enabled && root.showPeaks) {
                Audio.devices.input.peakValuesEnabled = true;
            }
        });
    }
    function disablePeakValues() {
        root.showPeaks = false;
        Audio.devices.input.peakValuesEnabled = false;
    }

    Component.onCompleted: enablePeakValues();
    Component.onDestruction: disablePeakValues();

    Column {
        y: 16; // padding does not work
        spacing: 16;
        width: parent.width;

        RalewayRegular {
            x: 16; // padding does not work
            size: 16;
            color: "white";
            text: root.title;

            visible: root.showTitle();
        }

        Separator { visible: root.showTitle() }

        ColumnLayout {
            x: 16; // padding does not work
            spacing: 16;

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
            HiFiGlyphs {
                text: hifi.glyphs.mic;
                color: hifi.colors.primaryHighlight;
                anchors.verticalCenter: parent.verticalCenter;
                size: 28;
            }
            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter;
                size: 16;
                color: hifi.colors.lightGrayText;
                text: qsTr("CHOOSE INPUT DEVICE");
            }
        }

        ListView {
            anchors { left: parent.left; right: parent.right; leftMargin: 70 }
            height: 125;
            spacing: 0;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.input;
            delegate: Item {
                width: parent.width;
                height: 36;

                RowLayout {
                    width: parent.width;

                    AudioControls.CheckBox {
                        Layout.maximumWidth: parent.width - inputPeak.width - 40;
                        text: display;
                        wrap: false;
                        checked: selected;
                        onClicked: {
                            selected = checked;
                            checked = Qt.binding(function() { return selected; }); // restore binding
                        }
                    }
                    InputPeak {
                        id: inputPeak;
                        visible: Audio.devices.input.peakValuesAvailable;
                        peak: model.peak;
                        Layout.alignment: Qt.AlignRight;
                        Layout.rightMargin: 30;
                    }
                }
            }
        }

        Separator {}

        RowLayout {
            Column {
                RowLayout {
                    HiFiGlyphs {
                        text: hifi.glyphs.unmuted;
                        color: hifi.colors.primaryHighlight;
                        anchors.verticalCenter: parent.verticalCenter;
                        size: 36;
                    }
                    RalewayRegular {
                        anchors.verticalCenter: parent.verticalCenter;
                        size: 16;
                        color: hifi.colors.lightGrayText;
                        text: qsTr("CHOOSE OUTPUT DEVICE");
                    }
                }

                PlaySampleSound { anchors { left: parent.left; leftMargin: 60 }}
            }
        }

        ListView {
            anchors { left: parent.left; right: parent.right; leftMargin: 70 }
            height: 125;
            spacing: 0;
            snapMode: ListView.SnapToItem;
            clip: true;
            model: Audio.devices.output;
            delegate: Item {
                width: parent.width;
                height: 36;
                AudioControls.CheckBox {
                    text: display;
                    checked: selected;
                    onClicked: {
                        selected = checked;
                        checked = Qt.binding(function() { return selected; }); // restore binding
                    }
                }
            }
        }
    }
}
