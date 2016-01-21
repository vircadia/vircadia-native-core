import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3

import "../styles"


FocusScope {
    id: root
    implicitHeight: border.height
    implicitWidth: border.width

    property alias currentItem: listView.currentItem
    property alias model: listView.model
    signal selected(var item)


    Border {
        id: border
        anchors.fill: listView
        anchors.margins: -8
        border.color: hifi.colors.hifiBlue
        color: hifi.colors.window
        // color: "#7f7f7f7f"
    }

    ListView {
        id: listView
        x: 8; y: 8
        HifiConstants { id: hifi }
        width: 128
        height: count * 32
        onEnabledChanged: recalcSize();
        onVisibleChanged: recalcSize();
        onCountChanged: recalcSize();
        focus: true

        highlight: Rectangle {
            width: listView.currentItem ? listView.currentItem.width : 0
            height: listView.currentItem ? listView.currentItem.height : 0
            color: "lightsteelblue"; radius: 3
        }

        delegate: VrMenuItem {
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



