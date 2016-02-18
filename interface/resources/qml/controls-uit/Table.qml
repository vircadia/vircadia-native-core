//
//  Table.qml
//
//  Created by David Rowe on 18 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

TableView {
    id: tableView

    property var tableModel: ListModel { }
    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    model: tableModel

    TableViewColumn {
        role: "name"
    }

    anchors { left: parent.left; right: parent.right }

    headerVisible: false

    // Use rectangle to draw border with rounded corners.
    frameVisible: false
    Rectangle {
        color: "#00000000"
        anchors { fill: parent; margins: -2 }
        radius: hifi.dimensions.borderRadius
        border.color: isLightColorScheme ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight
        border.width: 2
    }
    anchors.margins: 2  // Shrink TableView to lie within border.

    backgroundVisible: true

    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    verticalScrollBarPolicy: Qt.ScrollBarAsNeeded

    style: TableViewStyle {
        // Needed in order for rows to keep displaying rows after end of table entries.
        backgroundColor: parent.isLightColorScheme ? hifi.colors.tableRowLightEven : hifi.colors.tableRowDarkEven
        alternateBackgroundColor: parent.isLightColorScheme ? hifi.colors.tableRowLightOdd : hifi.colors.tableRowDarkOdd

        handle: Item {
            id: scrollbarHandle
            implicitWidth: 8
            Rectangle {
                radius: 4
                color: hifi.colors.tableScrollHandle
                anchors {
                    fill: parent
                    leftMargin: 4  // Finesse size and position.
                    rightMargin: -1
                    topMargin: 2
                    bottomMargin: 3
                }
            }
        }

        scrollBarBackground: Item {
            implicitWidth: 10

            Rectangle {
                color: hifi.colors.baseGrayHighlight
                anchors {
                    fill: parent
                    leftMargin: 1  // Finesse size and position.
                    topMargin: -2
                    bottomMargin: -2
                }
            }

            Rectangle {
                radius: 4
                color: hifi.colors.tableScrollBackground
                anchors {
                    fill: parent
                    leftMargin: 3  // Finesse position.
                }
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
        height: (styleData.selected ? 1.8 : 1) * hifi.dimensions.tableRowHeight
        color: styleData.selected
               ? hifi.colors.primaryHighlight
               : tableView.isLightColorScheme
                   ? (styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd)
                   : (styleData.alternate ? hifi.colors.tableRowDarkEven : hifi.colors.tableRowDarkOdd)
    }

    itemDelegate: Item {
        anchors {
            left: parent ? parent.left : undefined
            leftMargin: hifi.dimensions.tablePadding
            right: parent ? parent.right : undefined
            rightMargin: hifi.dimensions.tablePadding
        }

        FiraSansSemiBold {
            id: textItem
            text: styleData.value
            size: hifi.fontSizes.tableText
            color: colorScheme == hifi.colorSchemes.light
                       ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                       : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                topMargin: 3
            }

            // FIXME: Put reload item in tableModel passed in from RunningScripts.
            HiFiGlyphs {
                id: reloadButton
                text: "a"
                color: parent.color
                anchors {
                    top: parent.top
                    right: stopButton.left
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors { fill: parent; margins: -2 }
                    onClicked: reloadScript(model.url)
                }
            }

            // FIXME: Put stop item in tableModel passed in from RunningScripts.
            HiFiGlyphs {
                id: stopButton
                text: "C"
                color: parent.color
                anchors {
                    top: parent.top
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors { fill: parent; margins: -2 }
                    onClicked: stopScript(model.url)
                }
            }
        }

        // FIXME: Automatically use aux. information from tableModel
        FiraSansSemiBold {
            text: tableModel.get(styleData.row) ? tableModel.get(styleData.row).url : ""
            elide: Text.ElideMiddle
            size: hifi.fontSizes.tableText
            color: colorScheme == hifi.colorSchemes.light
                       ? (styleData.selected ? hifi.colors.black : hifi.colors.lightGray)
                       : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
            anchors {
                top: textItem.bottom
                left: parent.left
                right: parent.right
            }
            visible: styleData.selected
        }
    }
}
