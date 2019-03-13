//
//  Created by Dante Ruiz on 3/4/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


import QtQuick 2.5

import stylesUit 1.0
import controlsUit 1.0 as HifiControls


Rectangle {
    id: root
    anchors.fill: parent
    property string pluginName: ""
    property var displayInformation: null
    HifiConstants { id: hifi }

    color: hifi.colors.baseGray

    HifiControls.CheckBox {
        id: box
        width: 15
        height: 15

        anchors {
            left: root.left
            leftMargin: 75
        }

        onClicked: {
            sendConfigurationSettings( { trackControllersInOculusHome: checked });
        }
    }

    RalewaySemiBold {
        id: head

        text: "Track hand controllers in Oculus Home"
        size: 12

        color: "white"
        anchors.left: box.right
        anchors.leftMargin: 5
    }

    function displayConfiguration() {
        var configurationSettings = InputConfiguration.configurationSettings(root.pluginName);
        box.checked = configurationSettings.trackControllersInOculusHome;
    }

    function sendConfigurationSettings(settings) {
        InputConfiguration.setConfigurationSettings(settings, root.pluginName);
    }
}
