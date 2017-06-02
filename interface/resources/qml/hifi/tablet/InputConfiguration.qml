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

    Rectangle {
        width: inputConfiguration.width
        height: 1
        color: hifi.colors.baseGrayShadow
        x: -hifi.dimensions.contentMargin.x
    }

    RalewayBold {
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

    RalewayBold {
        id: configuration
        text: "SELECT DEVICE"
        size: 18
        color: hifi.colors.lightGray


        anchors.top: headerSeparator.bottom
        anchors.left: inputConfiguration.left
        anchors.leftMargin: 20
        anchors.topMargin: 20
    }

    Row {
        id: configRow

        anchors.top: configuration.bottom
        anchors.topMargin: 20
        anchors.left: configuration.left
        anchors.leftMargin: 40
        spacing: 10
        HifiControls.ComboBox {
            id: box
            width: 160
            
            colorScheme: hifi.colorSchemes.dark
            model: inputPlugins()
        }

        HifiControls.CheckBox {
            onClicked: {
                if (checked) {
                    console.log("button checked");
                    Tablet.getTablet("");
                    InputConfiguration.inputPlugins();
                }
            }
        }

    }

    function inputPlugins() {
        var plugins = ["temp"];
        return plugins
    }
}
