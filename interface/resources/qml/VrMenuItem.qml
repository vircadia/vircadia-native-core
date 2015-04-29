import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Item {
    id: root
    HifiConstants {
        id: hifi
    }

    property var source
    property var menuContainer
    property var listView

    MouseArea {
        anchors.left: parent.left
        anchors.right: tag.right
        anchors.rightMargin: -4
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true

        Rectangle {
            id: highlight
            visible: false
            anchors.fill: parent
            color: "#7f0e7077"
        }

        onEntered: {
            //if (source.type == 2 && enabled) {
            //    timer.start()
            //}
            highlight.visible = source.enabled
        }

        onExited: {
            timer.stop()
            highlight.visible = false
        }

        onClicked: {
            select()
        }
   }

    implicitHeight: label.implicitHeight * 1.5
    implicitWidth: label.implicitWidth + label.height * 2.5

    Timer {
        id: timer
        interval: 1000
        onTriggered: parent.select()
    }


    FontAwesome {
        clip: true
        id: check
        verticalAlignment:  Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
        text: checkText()
        size: label.height
        font.pixelSize: size
        function checkText() {
            if (!source || source.type != 1 || !source.checkable) {
                return ""
            }
            // FIXME this works for native QML menus but I don't think it will
            // for proxied QML menus
            if (source.exclusiveGroup) {
                return source.checked ? "\uF05D" : "\uF10C"
            }
            return source.checked ? "\uF046" : "\uF096"
        }
    }

    Text {
        id: label
        text: typedText()
        anchors.left: check.right
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        color: source.enabled ? hifi.colors.text : hifi.colors.disabledText
        enabled: source.enabled && source.visible
        function typedText() {
            if (source) {
                switch (source.type) {
                case 2:
                    return source.title
                case 1:
                    return source.text
                case 0:
                    return "-----"
                }
            }
            return ""
        }
    }

    FontAwesome {
        id: tag
        x: listView.width - width - 4
        size: label.height
        width: implicitWidth
        visible: source.type == 2
        text: "\uF0DA"
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
    }

    function select() {
        //timer.stop();
        var popped = false
        while (columns.length - 1 > listView.parent.menuDepth) {
            popColumn()
            popped = true
        }

        if (!popped || source.type != 1) {
            root.menuContainer.selectItem(source)
        }
    }
}
