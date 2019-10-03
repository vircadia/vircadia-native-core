import QtQuick 2.3
import QtQuick.Controls 2.1

Button {
    id: control
    property string backgroundColor: "#00000000"
    property string borderColor: "#FFFFFF"
    property string textColor: "#FFFFFF"
    property int backgroundOpacity: 1
    property int backgroundRadius: 1
    property int backgroundWidth: 2

    font.family: "Graphik Semibold"
    font.pointSize: 12

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        color: control.backgroundColor
        opacity: control.backgroundOpacity
        border.color: control.borderColor
        border.width: control.backgroundWidth
        radius: control.backgroundRadius
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
