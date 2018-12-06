//
//  ImageButton.qml
//  interface/resources/qml/controlsUit
//
//  Created by Gabriel Calero & Cristian Duarte on 12 Oct 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Layouts 1.3
import "../stylesUit" as HifiStyles

Item {
    id: button

    property string text: ""
    property string source : ""
    property string hoverSource : ""
    property real fontSize: 10
    property string fontColor: "#FFFFFF"
    property string hoverFontColor: "#000000"

    signal clicked();

    Rectangle {
        color: "transparent"
        anchors.fill: parent
        Image {
            id: image
            anchors.fill: parent
            source: button.source
        }

        HifiStyles.FiraSansRegular {
            id: buttonText
            anchors.centerIn: parent
            text: button.text
            color: button.fontColor
            font.pixelSize: button.fontSize
        }

        MouseArea {
            anchors.fill: parent
            onClicked: button.clicked();
            onEntered: {
                button.state = "hover state";
            }
            onExited: {
                button.state = "base state";
            }
        }


    }
    states: [
        State {
            name: "hover state"
            PropertyChanges {
                target: image
                source: button.hoverSource
            }
            PropertyChanges {
                target: buttonText
                color: button.hoverFontColor                    
            }
        },
        State {
            name: "base state"
            PropertyChanges {
                target: image
                source: button.source
            }
            PropertyChanges {
                target: buttonText
                color: button.fontColor                    
            }
        }
    ]
}
