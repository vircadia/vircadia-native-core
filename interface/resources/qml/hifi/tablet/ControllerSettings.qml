//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls

StackView {
    id: stack
    initialItem: inputConfiguration
    property alias messageVisible: imageMessageBox.visible
    Rectangle {
        id: inputConfiguration
        anchors.fill: parent

        HifiConstants { id: hifi }

        color: hifi.colors.baseGray

        property var pluginSettings: null

        HifiControls.ImageMessageBox {
            id: imageMessageBox
            anchors.fill: parent
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
            text: "Controller Settings"
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
                text: "show all input devices"

                onClicked: {
                    inputPlugins();
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

        Loader {
            id: loader
            asynchronous: false

            width: inputConfiguration.width
            anchors.left: inputConfiguration.left
            anchors.right: inputConfiguration.right
            anchors.top: configurationHeader.bottom
            anchors.topMargin: 10
            anchors.bottom: inputConfiguration.bottom

            source: InputConfiguration.configurationLayout(box.currentText);
            onLoaded: {
                if (loader.item.hasOwnProperty("pluginName")) {
                    if (box.currentText === "Vive") {
                        loader.item.pluginName = "OpenVR";
                    } else {
                        loader.item.pluginName = box.currentText;
                    }
                }

                if (loader.item.hasOwnProperty("displayInformation")) {
                    loader.item.displayConfiguration();
                }
            }
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
        var source = "";
        if (box.currentText == "Vive") {
            source = InputConfiguration.configurationLayout("OpenVR");
        } else {
            source = InputConfiguration.configurationLayout(box.currentText);
        }

        loader.source = source;
        if (source === "") {
            box.label = "(not configurable)";
        } else {
            box.label = "";
        }
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
