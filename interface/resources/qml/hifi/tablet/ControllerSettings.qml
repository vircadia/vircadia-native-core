//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0
import stylesUit 1.0
import "../../controls"
import controlsUit 1.0 as HifiControls
import "../../dialogs"
import "../../dialogs/preferences"
import "tabletWindows"
import "../audio"

Item {
    id: controllerSettings
    height: parent.height
    width: parent.width

    property string title: "Controls"
    property var openVRDevices: ["HTC Vive", "Valve Index", "Valve HMD", "Valve"]

    HifiConstants { id: hifi }

    TabBar {
        id: bar
        spacing: 0
        anchors.top: controllerSettings.top
        width: parent.width
        height: 40
        z: 10

        TabButton {
            height: parent.height
            text: qsTr("Settings")
            onClicked: {
                stackView.clear();
                stackView.push(controllerPreferencesComponent);
            }
        }
        TabButton {
            height: parent.height
            text: qsTr("Calibration")
            onClicked: {
                stackView.clear();
                stackView.push(inputConfigurationComponent);
            }
        }
    }


    StackView {
        id: stackView
        anchors.top: bar.bottom
        anchors.bottom: controllerSettings.bottom
        anchors.left: controllerSettings.left
        anchors.right: controllerSettings.right

        initialItem: controllerPreferencesComponent
    }

    Component {
        id: inputConfigurationComponent
        StackView {
            id: stack
            initialItem: inputConfiguration
            property alias messageVisible: imageMessageBox.visible
            property string selectedPlugin: ""

            property bool keyboardEnabled: false
            property bool keyboardRaised: false
            property bool punctuationMode: false

            Rectangle {
                id: inputConfiguration
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }

                height: 230

                HifiConstants { id: hifi }

                color: hifi.colors.baseGray

                property var pluginSettings: null

                HifiControls.ImageMessageBox {
                    id: imageMessageBox
                    anchors.top: parent.top
                    anchors.topMargin: 444
                    z: 2000
                    imageWidth: 442
                    imageHeight: 670
                    source: "../../../images/calibration-help.png"
                }

                Rectangle {
                    width: inputConfiguration.width
                    height: 1
                    color: hifi.colors.baseGrayShadow
                    x: -hifi.dimensions.contentMargin.x
                }

                RalewayRegular {
                    id: header
                    text: "Control Settings"
                    size: 22
                    color: "white"

                    anchors.top: inputConfiguration.top
                    anchors.left: inputConfiguration.left
                    anchors.leftMargin: 20
                    anchors.topMargin: 20
                }

                Separator {
                    id: headerSeparator
                    width: inputConfiguration.width
                    anchors.top: header.bottom
                    anchors.topMargin: 10
                }

                HiFiGlyphs {
                    id: sourceGlyph
                    text: hifi.glyphs.source
                    size: 36
                    color: hifi.colors.blueHighlight

                    anchors.top: headerSeparator.bottom
                    anchors.left: inputConfiguration.left
                    anchors.leftMargin: 40
                    anchors.topMargin: 20
                }

                RalewayRegular {
                    id: configuration
                    text: "SELECT DEVICE"
                    size: 15
                    color: hifi.colors.lightGrayText


                    anchors.top: headerSeparator.bottom
                    anchors.left: sourceGlyph.right
                    anchors.leftMargin: 10
                    anchors.topMargin: 30
                }

                Row {
                    id: configRow
                    z: 999
                    anchors.top: sourceGlyph.bottom
                    anchors.topMargin: 20
                    anchors.left: sourceGlyph.left
                    anchors.leftMargin: 40
                    spacing: 10
                    HifiControls.ComboBox {
                        id: box
                        width: 160
                        z: 999
                        editable: true
                        colorScheme: hifi.colorSchemes.dark
                        model: inputPlugins()
                        label: ""

                        onCurrentIndexChanged: {
                            changeSource();
                        }
                    }

                    HifiControls.CheckBox {
                        id: checkBox
                        colorScheme: hifi.colorSchemes.dark
                        text: "Show all input devices"

                        onClicked: {
                            box.model = inputPlugins();
                            changeSource();
                        }
                    }
                }


                Separator {
                    id: configurationSeparator
                    z: 0
                    width: inputConfiguration.width
                    anchors.top: configRow.bottom
                    anchors.topMargin: 10
                }


                HiFiGlyphs {
                    id: sliderGlyph
                    text: hifi.glyphs.sliders
                    size: 36
                    color: hifi.colors.blueHighlight

                    anchors.top: configurationSeparator.bottom
                    anchors.left: inputConfiguration.left
                    anchors.leftMargin: 40
                    anchors.topMargin: 20
                }

                RalewayRegular {
                    id: configurationHeader
                    text: "CONFIGURATION"
                    size: 15
                    color: hifi.colors.lightGrayText


                    anchors.top: configurationSeparator.bottom
                    anchors.left: sliderGlyph.right
                    anchors.leftMargin: 10
                    anchors.topMargin: 30
                }
            }

            Rectangle {
                id: loaderRectangle
                z: -1
                color: hifi.colors.baseGray
                width: parent.width
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: inputConfiguration.bottom
                anchors.bottom: parent.bottom
                anchors.bottomMargin: keyboard.height

                Loader {
                    id: loader
                    asynchronous: false
                    anchors.fill: parent
                    source: InputConfiguration.configurationLayout(box.textAt(box.currentIndex));
                    onLoaded: {
                        if (loader.item.hasOwnProperty("pluginName")) {
                            if (openVRDevices.indexOf(box.textAt(box.currentIndex)) !== -1) {
                                loader.item.pluginName = "OpenVR";
                            } else {
                                loader.item.pluginName = box.textAt(box.currentIndex);
                            }
                        }

                        if (loader.item.hasOwnProperty("displayInformation")) {
                            loader.item.displayConfiguration();
                        }
                    }
                }
            }

            HifiControls.Keyboard {
                id: keyboard
                raised: parent.keyboardEnabled && parent.keyboardRaised
                onRaisedChanged: {
                    if (raised) {
                        // delayed execution to allow loader and its content to adjust size
                        Qt.callLater(function() {
                            loader.item.bringToView(Window.activeFocusItem);
                        })
                    }
                }

                numeric: parent.punctuationMode
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }

                Component.onCompleted: {
                    parent.keyboardEnabled = HMD.active;
                }
            }

            function inputPlugins() {
                if (checkBox.checked) {
                    return InputConfiguration.inputPlugins();
                } else {
                    return InputConfiguration.activeInputPlugins();
                }
            }

            function initialize() {
                changeSource();
            }

            function changeSource() {
                loader.source = "";
                var selectedDevice = box.textAt(box.currentIndex);
                var source = "";
                if (openVRDevices.indexOf(selectedDevice) !== -1) {
                    source = InputConfiguration.configurationLayout("OpenVR");
                } else {
                    source = InputConfiguration.configurationLayout(selectedDevice);
                }

                loader.source = source;
                if (source === "") {
                    box.label = "(not configurable)";
                } else {
                    box.label = "";
                }

                stack.selectedPlugin = selectedDevice;
            }

            Timer {
                id: timer
                repeat: false
                interval: 300
                onTriggered: initialize()
            }

            Component.onCompleted: {
                timer.start();
            }
        }
    }


    Component {
        id: controllerPreferencesComponent
        TabletPreferencesDialog {
            anchors.fill: stackView
            id: controllerPrefereneces
            objectName: "TabletControllerPreferences"
            showCategories: ["VR Movement", "Game Controller", "Sixense Controllers", "Perception Neuron", "Leap Motion", "Open Sound Control (OSC)"]
            categoryProperties: {
                "VR Movement" : {
                    "User real-world height (meters)" : { "anchors.right" : "undefined" },
                    "RESET SENSORS" : { "width" : "180", "anchors.left" : "undefined" }
                }
            }
        }
    }
}
