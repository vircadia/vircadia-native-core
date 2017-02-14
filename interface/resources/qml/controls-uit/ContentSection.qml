//
//  ContentSection.qml
//
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

import "../styles-uit"

Column {
    property string name: "Content Section"
    property bool isFirst: false
    property bool isCollapsible: false  // Set at creation.
    property bool isCollapsed: false

    spacing: 0  // Defer spacing decisions to individual controls.

    anchors {
        left: parent.left
        leftMargin: hifi.dimensions.contentMargin.x
        right: parent.right
        rightMargin: hifi.dimensions.contentMargin.x
    }

    function toggleCollapsed() {
        if (isCollapsible) {
            isCollapsed = !isCollapsed;
            for (var i = 1; i < children.length; i++) {
                children[i].visible = !isCollapsed;
            }
        }
    }

    Item {
        id: sectionName
        anchors.left: parent.left
        anchors.right: parent.right
        height: leadingSpace.height + topBar.height + heading.height + bottomBar.height

        Item {
            id: leadingSpace
            width: 1
            height: isFirst ? 7 : 0
            anchors.top: parent.top
        }

        Item {
            id: topBar
            visible: !isFirst
            height: visible ? 2 : 0
            anchors.top: leadingSpace.bottom

            Rectangle {
                id: shadow
                width: frame.width
                height: 1
                color: hifi.colors.baseGrayShadow
                x: -hifi.dimensions.contentMargin.x
            }

            Rectangle {
                width: frame.width
                height: 1
                color: hifi.colors.baseGrayHighlight
                x: -hifi.dimensions.contentMargin.x
                anchors.top: shadow.bottom
            }
        }

        Item {
            id: heading
            anchors {
                left: parent.left
                right: parent.right
                top: topBar.bottom
            }
            height: isCollapsible ? 36 : 28

            RalewayRegular {
                id: title
                anchors {
                    left: parent.left
                    top: parent.top
                    topMargin: 12
                }
                size: hifi.fontSizes.sectionName
                font.capitalization: Font.AllUppercase
                text: name
                color: hifi.colors.lightGrayText
            }

            HiFiGlyphs {
                anchors {
                    top: title.top
                    topMargin: -9
                    right: parent.right
                    rightMargin: -4
                }
                size: hifi.fontSizes.disclosureButton
                text: isCollapsed ? hifi.glyphs.disclosureButtonExpand : hifi.glyphs.disclosureButtonCollapse
                color: hifi.colors.lightGrayText
                visible: isCollapsible
            }

            MouseArea {
                // Events are propogated so that any active control is defocused.
                anchors.fill: parent
                propagateComposedEvents: true
                onPressed: {
                    toggleCollapsed();
                    mouse.accepted = false;
                }
            }
        }

        LinearGradient {
            id: bottomBar
            visible: desktop.gradientsSupported && isCollapsible
            width: frame.width
            height: visible ? 4 : 0
            x: -hifi.dimensions.contentMargin.x
            anchors.top: heading.bottom
            start: Qt.point(0, 0)
            end: Qt.point(0, 4)
            gradient: Gradient {
                GradientStop { position: 0.0; color: hifi.colors.darkGray }
                GradientStop { position: 1.0; color: hifi.colors.baseGray }  // Equivalent of darkGray0 over baseGray background.
            }
            cached: true
        }
    }
}
