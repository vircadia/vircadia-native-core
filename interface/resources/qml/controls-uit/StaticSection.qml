//
//  StaticSection.qml
//
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../styles-uit"

Column {
    property string name: "Static Section"
    property bool hasSeparator: false

    spacing: hifi.dimensions.contentSpacing.y

    anchors {
        left: parent.left
        leftMargin: hifi.dimensions.contentMargin.x
        right: parent.right
        rightMargin: hifi.dimensions.contentMargin.x
    }

    VerticalSpacer { }

    Item {
        visible: hasSeparator
        anchors.top: sectionName.top

        Rectangle {
            width: frame.width
            height: 1
            color: hifi.colors.baseGrayShadow
            x: -hifi.dimensions.contentMargin.x
            anchors.bottom: highlight.top
        }

        Rectangle {
            id: highlight
            width: frame.width
            height: 1
            color: hifi.colors.baseGrayHighlight
            x: -hifi.dimensions.contentMargin.x
            anchors.bottom: parent.top
        }
    }

    RalewayRegular {
        id: sectionName
        text: parent.name
        size: hifi.fontSizes.sectionName
        font.capitalization: Font.AllUppercase
        color: hifi.colors.lightGrayText
        verticalAlignment: Text.AlignBottom
        height: {
            if (hasSeparator) {
                hifi.dimensions.contentMargin.y
            }
        }
    }
}
