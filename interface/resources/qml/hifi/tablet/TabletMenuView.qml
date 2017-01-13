//
//  VrMenuView.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../../styles-uit"

FocusScope {
    id: root
    implicitHeight: background.height
    implicitWidth: background.width

    property alias currentItem: listView.currentItem
    property alias model: listView.model
    property bool isSubMenu: false
    signal selected(var item)

    HifiConstants { id: hifi }

    Rectangle {
        id: background
        anchors.fill: listView
        radius: hifi.dimensions.borderRadius
        border.width: hifi.dimensions.borderWidth
        border.color: hifi.colors.lightGrayText80
        color: hifi.colors.faintGray
        //color: isSubMenu ? hifi.colors.faintGray : hifi.colors.faintGray80
    }

    ListView {
        id: listView
        x: 0
        y: 0
        width: 480
        height: 720

        topMargin: hifi.dimensions.menuPadding.y + 90
        onEnabledChanged: recalcSize();
        onVisibleChanged: recalcSize();
        onCountChanged: recalcSize();
        focus: true
        highlightMoveDuration: 0

        highlight: Rectangle {
            anchors {
                left: parent ? parent.left : undefined
                right: parent ? parent.right : undefined
                leftMargin: hifi.dimensions.borderWidth
                rightMargin: hifi.dimensions.borderWidth
            }
            color: hifi.colors.white
        }

        delegate: TabletMenuItem {
            text: name
            source: item
            onImplicitHeightChanged: listView.recalcSize()
            onImplicitWidthChanged: listView.recalcSize()

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: listView.currentIndex = index
                onClicked: root.selected(item)
            }
        }

        function recalcSize() {
            if (model.count !== count || !visible) {
                return;
            }

            var originalIndex = currentIndex;
            var maxWidth = width;
            var newHeight = 0;
            for (var i = 0; i < count; ++i) {
                currentIndex = i;
                if (!currentItem) {
                    continue;
                }
                if (currentItem && currentItem.implicitWidth > maxWidth) {
                    maxWidth = currentItem.implicitWidth
                }
                if (currentItem.visible) {
                    newHeight += currentItem.implicitHeight
                }
            }
            newHeight += 2 * hifi.dimensions.menuPadding.y;  // White space at top and bottom.
            if (maxWidth > width) {
                width = maxWidth;
            }
            if (newHeight > height) {
                height = newHeight
            }
            currentIndex = originalIndex;
        }

        function previousItem() { currentIndex = (currentIndex + count - 1) % count; }
        function nextItem() { currentIndex = (currentIndex + count + 1) % count; }
        function selectCurrentItem() { if (currentIndex != -1) root.selected(currentItem.source); }

        Keys.onUpPressed: previousItem();
        Keys.onDownPressed: nextItem();
        Keys.onSpacePressed: selectCurrentItem();
        Keys.onRightPressed: selectCurrentItem();
        Keys.onReturnPressed: selectCurrentItem();
    }
}



