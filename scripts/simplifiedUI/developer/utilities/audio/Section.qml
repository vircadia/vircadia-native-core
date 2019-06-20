//
//  Section.qml
//  scripts/developer/utilities/audio
//
//  Created by Zach Pomerantz on 9/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

Rectangle {
    id: section 
    property string label: "Section"
    property string description: "Description"
    property alias control : loader.sourceComponent

    width: parent.width
    height: content.height + border.width * 2 + content.spacing * 2
    border.color: "black"
    border.width: 5
    radius: border.width * 2

    ColumnLayout {
        id: content
        x: section.radius; y: section.radius
        spacing: section.border.width
        width: section.width - 2 * x

        // label
        Label {
            Layout.alignment: Qt.AlignCenter
            text: hoverArea.containsMouse ? section.description : section.label
            font.bold: true

            MouseArea {
                id: hoverArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }

        // spacer
        Item { }

        // control
        Loader {
            id: loader
            Layout.preferredWidth: parent.width
        }
    }
}

