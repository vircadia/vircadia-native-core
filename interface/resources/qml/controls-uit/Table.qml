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

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property bool expandSelectedRow: false
    property bool centerHeaderText: false

    model: ListModel { }

    headerVisible: false
    headerDelegate: Rectangle {
        height: hifi.dimensions.tableHeaderHeight
        color: isLightColorScheme ? hifi.colors.tableBackgroundLight : hifi.colors.tableBackgroundDark

        RalewayRegular {
            id: titleText
            text: styleData.value
            size: hifi.fontSizes.tableHeading
            font.capitalization: Font.AllUppercase
            color: hifi.colors.baseGrayHighlight
            horizontalAlignment: (centerHeaderText ? Text.AlignHCenter : Text.AlignLeft)
            anchors {
                left: parent.left
                leftMargin: hifi.dimensions.tablePadding
                right: parent.right
                rightMargin: hifi.dimensions.tablePadding
                verticalCenter: parent.verticalCenter
            }
        }

        HiFiGlyphs {
            id: titleSort
            text:  sortIndicatorOrder == Qt.AscendingOrder ? hifi.glyphs.caratUp : hifi.glyphs.caratDn
            color: hifi.colors.baseGrayHighlight
            size: hifi.fontSizes.tableHeadingIcon
            anchors {
                left: titleText.right
                leftMargin: -hifi.fontSizes.tableHeadingIcon / 3 - (centerHeaderText ? 3 : 0)
                right: parent.right
                rightMargin: hifi.dimensions.tablePadding
                verticalCenter: titleText.verticalCenter
            }
            visible: sortIndicatorVisible && sortIndicatorColumn === styleData.column
        }

        Rectangle {
            width: 1
            anchors {
                left: parent.left
                top: parent.top
                topMargin: 1
                bottom: parent.bottom
                bottomMargin: 2
            }
            color: isLightColorScheme ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight
            visible: styleData.column > 0
        }

        Rectangle {
            height: 1
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            color: isLightColorScheme ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight
        }
    }

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
        backgroundColor: tableView.isLightColorScheme ? hifi.colors.tableBackgroundLight : hifi.colors.tableBackgroundDark
        alternateBackgroundColor: tableView.isLightColorScheme ? hifi.colors.tableRowLightOdd : hifi.colors.tableRowDarkOdd

        padding.top: headerVisible ? hifi.dimensions.tableHeaderHeight: 0

        handle: Item {
            id: scrollbarHandle
            implicitWidth: 6
            Rectangle {
                anchors {
                    fill: parent
                    topMargin: 3
                    bottomMargin: 3     // ""
                    leftMargin: 2       // Move it right
                    rightMargin: -2     // ""
                }
                radius: 3
                color: isLightColorScheme ? hifi.colors.tableScrollHandleLight : hifi.colors.tableScrollHandleDark
            }
        }

        scrollBarBackground: Item {
            implicitWidth: 9
            Rectangle {
                anchors {
                    fill: parent
                    margins: -1     // Expand
                    topMargin: headerVisible ? -hifi.dimensions.tableHeaderHeight : -1
                }
                color: isLightColorScheme ? hifi.colors.tableBackgroundLight : hifi.colors.tableBackgroundDark

                Rectangle {
                    // Extend header bottom border
                    anchors {
                        top: parent.top
                        topMargin: hifi.dimensions.tableHeaderHeight - 1
                        left: parent.left
                        right: parent.right
                    }
                    height: 1
                    color: isLightColorScheme ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight
                    visible: headerVisible
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
        height: (styleData.selected && expandSelectedRow ? 1.8 : 1) * hifi.dimensions.tableRowHeight
        color: styleData.selected
               ? hifi.colors.primaryHighlight
               : tableView.isLightColorScheme
                   ? (styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd)
                   : (styleData.alternate ? hifi.colors.tableRowDarkEven : hifi.colors.tableRowDarkOdd)
    }
}
