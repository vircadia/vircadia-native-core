//
//  AttachmentsTable.qml
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
import QtQuick.XmlListModel 2.0

import "../styles-uit"
import "../controls-uit" as HifiControls
import "../windows"
import "../hifi/models"

TableView {
    id: tableView

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    model: S3Model{}

    Rectangle {
        anchors.fill: parent
        visible: tableView.model.status !== XmlListModel.Ready
        color: hifi.colors.darkGray0
        BusyIndicator {
            anchors.centerIn: parent
            width: 48; height: 48
            running: true
        }
    }

    headerDelegate: Rectangle {
        height: hifi.dimensions.tableHeaderHeight
        color: hifi.colors.darkGray
        border.width: 0.5
        border.color: hifi.colors.baseGrayHighlight

        RalewayRegular {
            id: textHeader
            size: hifi.fontSizes.tableHeading
            color: hifi.colors.lightGrayText
            text: styleData.value
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
        }
    }

    // Use rectangle to draw border with rounded corners.
    Rectangle {
        color: "#00000000"
        anchors { fill: parent; margins: -2 }
        radius: hifi.dimensions.borderRadius
        border.color: hifi.colors.baseGrayHighlight
        border.width: 3
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
                color: hifi.colors.tableScrollHandleDark
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
                color: hifi.colors.tableScrollBackgroundDark
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
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
        }
    }

    TableViewColumn {
        role: "name"
        title: "NAME"
        width: parent.width *0.3
        horizontalAlignment: Text.AlignHCenter
    }
    TableViewColumn {
        role: "size"
        title: "SIZE"
        width: parent.width *0.2
        horizontalAlignment: Text.AlignHCenter
    }
    TableViewColumn {
        role: "modified"
        title: "LAST MODIFIED"
        width: parent.width *0.5
        horizontalAlignment: Text.AlignHCenter
    }
}
