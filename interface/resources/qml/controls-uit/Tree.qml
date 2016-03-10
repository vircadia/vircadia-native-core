//
//  Table.qml
//
//  Created by David Rowe on 17 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

TreeView {
    id: treeView

    property var treeModel: ListModel { }
    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    model: treeModel

    TableViewColumn {
        role: "display";
    }

    anchors { left: parent.left; right: parent.right }

    headerVisible: false
    headerDelegate: Item { }  // Fix OSX QML bug that displays scrollbar starting too low.

    // Use rectangle to draw border with rounded corners.
    frameVisible: false
    Rectangle {
        color: "#00000000"
        anchors.fill: parent
        radius: hifi.dimensions.borderRadius
        border.color: isLightColorScheme ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight
        border.width: 2
        anchors.margins: -2
    }
    anchors.margins: 2  // Shrink TreeView to lie within border.

    backgroundVisible: true

    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    verticalScrollBarPolicy: Qt.ScrollBarAsNeeded

    style: TreeViewStyle {
        // Needed in order for rows to keep displaying rows after end of table entries.
        backgroundColor: parent.isLightColorScheme ? hifi.colors.tableRowLightEven : hifi.colors.tableRowDarkEven
        alternateBackgroundColor: parent.isLightColorScheme ? hifi.colors.tableRowLightOdd : hifi.colors.tableRowDarkOdd

        branchDelegate: HiFiGlyphs {
            text: styleData.isExpanded ? hifi.glyphs.caratDn : hifi.glyphs.caratR
            size: hifi.fontSizes.carat
            color: colorScheme == hifi.colorSchemes.light
                   ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                   : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
            anchors {
                left: parent ? parent.left : undefined
                leftMargin: hifi.dimensions.tablePadding / 2
            }
        }

        handle: Item {
            id: scrollbarHandle
            implicitWidth: 6
            Rectangle {
                anchors {
                    fill: parent
                    leftMargin: 2       // Move it right
                    rightMargin: -2     // ""
                    topMargin: 3        // Shrink vertically
                    bottomMargin: 3     // ""
                }
                radius: 3
                color: hifi.colors.tableScrollHandle
            }
        }

        scrollBarBackground: Item {
            implicitWidth: 10
            Rectangle {
                anchors {
                    fill: parent
                    margins: -1     // Expand
                }
                color: hifi.colors.baseGrayHighlight
            }

            Rectangle {
                anchors {
                    fill: parent
                    margins: 1      // Shrink
                }
                radius: 4
                color: hifi.colors.tableScrollBackground
            }
        }

        incrementControl: Item {
            visible: false
        }

        decrementControl: Item {
            visible: false
        }
    }

    rowDelegate: Rectangle {
        height: hifi.dimensions.tableRowHeight
        color: styleData.selected
               ? hifi.colors.primaryHighlight
               : treeView.isLightColorScheme
                   ? (styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd)
                   : (styleData.alternate ? hifi.colors.tableRowDarkEven : hifi.colors.tableRowDarkOdd)
    }

    itemDelegate: FiraSansSemiBold {
        anchors {
            left: parent ? parent.left : undefined
            leftMargin: (2 + styleData.depth) * hifi.dimensions.tablePadding
            right: parent ? parent.right : undefined
            rightMargin: hifi.dimensions.tablePadding
            verticalCenter: parent ? parent.verticalCenter : undefined
        }

        text: styleData.value
        size: hifi.fontSizes.tableText
        color: colorScheme == hifi.colorSchemes.light
                   ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                   : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
    }

    onDoubleClicked: isExpanded(index) ? collapse(index) : expand(index)

    onActivated: {
        var path = scriptsModel.data(index, 0x100)
        if (path) {
            loadScript(path)
        }
    }
}
