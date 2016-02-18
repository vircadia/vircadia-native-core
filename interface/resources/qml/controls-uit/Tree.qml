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

    style: TreeViewStyle {
        // Needed in order for rows to keep displaying rows after end of table entries.
        backgroundColor: treeView.isLightColorScheme ? hifi.colors.tableRowLightEven : hifi.colors.tableRowDarkEven
        alternateBackgroundColor: treeView.isLightColorScheme ? hifi.colors.tableRowLightOdd : hifi.colors.tableRowDarkOdd
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
            left: parent.left
            leftMargin: (2 + styleData.depth) * hifi.dimensions.tablePadding
            right: parent.right
            rightMargin: hifi.dimensions.tablePadding
            verticalCenter: parent.verticalCenter
        }

        text: styleData.value
        size: hifi.fontSizes.tableText
        color: colorScheme == hifi.colorSchemes.light
                   ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                   : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
    }

    onDoubleClicked: isExpanded(index) ? collapse(index) : expand(index)

    // FIXME not triggered by double click?
    onActivated: {
        var path = scriptsModel.data(index, 0x100)
        if (path) {
            loadScript(path)
        }
    }
}
