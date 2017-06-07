//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls

Rectangle {
    id: inputConfiguration
    anchors.fill: parent

    HifiConstants { id: hifi }

    color: hifi.colors.baseGray

    property var pluginSettings: null

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
        anchors.verticalCenter: sourceGlyph.horizontalCenter
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
            
            onCurrentIndexChanged: {
                loader.source = ""
                loader.source = InputConfiguration.configurationLayout(box.currentText);
            }
        }

        HifiControls.CheckBox {
            onClicked: {
                if (checked) {
                    box.model = InputConfiguration.activeInputPlugins();
                    loader.source = "";
                    loader.source = InputConfiguration.configurationLayout(box.currentText);
                } else {
                    box.model = InputConfiguration.inputPlugins();
                    loader.source = "";
                    loader.source = InputConfiguration.configurationLayout(box.currentText);
                }
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
        anchors.verticalCenter: sliderGlyph.horizontalCenter
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


        onLoaded: {
            if (loader.item.hasOwnProperty("pluginName")) {
                loader.item.pluginName = box.currentText
            }
        }
    }

    function inputPlugins() {
        return InputConfiguration.inputPlugins();
    }
}
