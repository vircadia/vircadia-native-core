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

    // The model object
    property alias text: label.text
    property var source

    implicitHeight: source.visible ? label.implicitHeight * 1.5 : 0
    implicitWidth: label.width + label.height * 2.5
    visible: source.visible
    width: parent.width

    FontAwesome {
        clip: true
        id: check
        verticalAlignment:  Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
        text: checkText()
        size: label.height
        visible: source.visible
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
        anchors.left: check.right
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        color: source.enabled ? hifi.colors.text : hifi.colors.disabledText
        enabled: source.visible && (source.type !== 0 ? source.enabled : false)
        visible: source.visible
    }

    FontAwesome {
        id: tag
        x: root.parent.width - width
        size: label.height
        width: implicitWidth
        visible: source.visible && (source.type == 2)
        text: "\uF0DA"
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
    }
}
