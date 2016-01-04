import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3

import "styles"

ListView {
    id: root
    HifiConstants { id: hifi }
    width: 128
    height: count * 32
    onEnabledChanged: recalcSize();
    onVisibleChanged: recalcSize();
    onCountChanged: recalcSize();

    signal selected(var item)

    highlight: Rectangle {
        width: root.currentItem ? root.currentItem.width : 0
        height: root.currentItem ? root.currentItem.height : 0
        color: "lightsteelblue"; radius: 3
    }

    delegate: VrMenuItem {
        text: name
        source: item
        onImplicitHeightChanged: root.recalcSize()
        onImplicitWidthChanged: root.recalcSize()

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: root.currentIndex = index
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
        if (maxWidth > width) {
            width = maxWidth;
        }
        if (newHeight > height) {
            height = newHeight
        }
        currentIndex = originalIndex;
    }

    Border {
        id: border
        anchors.fill: parent
        anchors.margins: -8
        z: parent.z - 1
        border.color: hifi.colors.hifiBlue
        color: hifi.colors.window
    }
}


