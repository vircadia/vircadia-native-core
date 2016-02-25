import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "." as VrControls

FocusScope {
    id: root
    property alias model: comboBox.model;
    readonly property alias currentText: comboBox.currentText;
    property alias currentIndex: comboBox.currentIndex;
    implicitHeight: comboBox.height;
    focus: true

    readonly property ComboBox control: comboBox

    Rectangle {
        id: background
        gradient: Gradient {
            GradientStop {color: control.pressed ? "#bababa" : "#fefefe" ; position: 0}
            GradientStop {color: control.pressed ? "#ccc" : "#e3e3e3" ; position: 1}
        }
        anchors.fill: parent
        border.color: control.activeFocus ? "#47b" : "#999"
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: control.activeFocus ? "#47b" : "white"
            opacity: control.hovered || control.activeFocus ? 0.1 : 0
            Behavior on opacity {NumberAnimation{ duration: 100 }}
        }
    }

    SystemPalette { id: palette }

    ComboBox {
        id: comboBox
        anchors.fill: parent
        visible: false
    }

    Text {
        id: textField
        anchors { left: parent.left; leftMargin: 2; right: dropIcon.left; verticalCenter: parent.verticalCenter }
        text: comboBox.currentText
        elide: Text.ElideRight
    }

    Item {
        id: dropIcon
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        width: 20
        height: textField.height
        VrControls.FontAwesome {
            anchors.centerIn: parent; size: 16;
            text: "\uf0d7"
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: toggleList();
    }

    function toggleList() {
        if (popup.visible) {
            hideList();
        } else {
            showList();
        }
    }

    function showList() {
        var r = desktop.mapFromItem(root, 0, 0, root.width, root.height);
        listView.currentIndex = root.currentIndex
        scrollView.x = r.x;
        scrollView.y = r.y + r.height;
        var bottom = scrollView.y + scrollView.height;
        if (bottom > desktop.height) {
            scrollView.y -= bottom - desktop.height + 8;
        }
        popup.visible = true;
        popup.forceActiveFocus();
    }

    function hideList() {
        popup.visible = false;
    }

    FocusScope {
        id: popup
        parent: desktop
        anchors.fill: parent
        z: desktop.zLevels.menu
        visible: false
        focus: true

        MouseArea {
            anchors.fill: parent
            onClicked: hideList();
        }

        function previousItem() { listView.currentIndex = (listView.currentIndex + listView.count - 1) % listView.count; }
        function nextItem() { listView.currentIndex = (listView.currentIndex + listView.count + 1) % listView.count; }
        function selectCurrentItem() { root.currentIndex = listView.currentIndex; hideList(); }

        Keys.onUpPressed: previousItem();
        Keys.onDownPressed: nextItem();
        Keys.onSpacePressed: selectCurrentItem();
        Keys.onRightPressed: selectCurrentItem();
        Keys.onReturnPressed: selectCurrentItem();
        Keys.onEscapePressed: hideList();

        ScrollView {
            id: scrollView
            height: 480
            width: root.width

            ListView {
                id: listView
                height: textField.height * count * 1.4
                model: root.model
                highlight: Rectangle{
                    width: listView.currentItem ? listView.currentItem.width : 0
                    height: listView.currentItem ? listView.currentItem.height : 0
                    color: "red"
                }
                delegate: Rectangle {
                    width: root.width
                    height: popupText.implicitHeight * 1.4
                    color: ListView.isCurrentItem ? palette.highlight : palette.base
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        id: popupText
                        x: 3
                        text: listView.model[index]
                    }
                    MouseArea {
                        id: popupHover
                        anchors.fill: parent;
                        hoverEnabled: true
                        onEntered: listView.currentIndex = index;
                        onClicked: popup.selectCurrentItem()
                    }
                }
            }
        }
    }

}
