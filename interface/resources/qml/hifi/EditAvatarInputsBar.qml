//
//  EditAvatarInputsBar.qml
//  qml/hifi
//
//  Audio setup
//
//  Created by Wayne Chen on 3/20/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../windows"

Rectangle {
    id: editRect

    HifiConstants { id: hifi; }

    color: hifi.colors.baseGray;

    signal sendToScript(var message);
    function emitSendToScript(message) {
        console.log("sending to script");
        console.log(JSON.stringify(message));
        sendToScript(message);
    }

    function fromScript(message) {
    }

    RalewayRegular {
        id: title;
        color: hifi.colors.white;
        text: qsTr("Avatar Inputs Persistent UI Settings")
        size: 20
        font.bold: true
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: (parent.width - width) / 2
        }
    }

    HifiControlsUit.Slider {
        id: xSlider
        anchors {
            top: title.bottom
            topMargin: 50
            left: parent.left
            leftMargin: 20
        }
        label: "X OFFSET"
        maximumValue: 1.0
        minimumValue: -1.0
        stepSize: 0.05
        value: -0.2
        width: 300
        onValueChanged: {
            emitSendToScript({
                "method": "reposition",
                "x": value
            });
        }
    }

    HifiControlsUit.Slider {
        id: ySlider
        anchors {
            top: xSlider.bottom
            topMargin: 50
            left: parent.left
            leftMargin: 20
        }
        label: "Y OFFSET"
        maximumValue: 1.0
        minimumValue: -1.0
        stepSize: 0.05
        value: -0.125
        width: 300
        onValueChanged: {
            emitSendToScript({
                "method": "reposition",
                "y": value
            });
        }
    }

    HifiControlsUit.Slider {
        id: zSlider
        anchors {
            top: ySlider.bottom
            topMargin: 50
            left: parent.left
            leftMargin: 20
        }
        label: "Z OFFSET"
        maximumValue: 0.0
        minimumValue: -1.0
        stepSize: 0.05
        value: -0.5
        width: 300
        onValueChanged: {
            emitSendToScript({
                "method": "reposition",
                "z": value
            });
        }
    }

    HifiControlsUit.Button {
        id: setVisibleButton;
        text: setVisible ? "SET INVISIBLE" : "SET VISIBLE";
        width: 300;
        property bool setVisible: true;
        anchors {
            top: zSlider.bottom
            topMargin: 50
            left: parent.left
            leftMargin: 20
        }
        onClicked: {
            setVisible = !setVisible;
            emitSendToScript({
                "method": "setVisible",
                "visible": setVisible
            });
        }
    }

    HifiControlsUit.Button {
        id: printButton;
        text: "PRINT POSITIONS";
        width: 300;
        anchors {
            top: setVisibleButton.bottom
            topMargin: 50
            left: parent.left
            leftMargin: 20
        }
        onClicked: {
            emitSendToScript({
                "method": "print",
            });
        }
    }
}
