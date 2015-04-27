import QtQuick 2.3
import QtQuick.Controls 1.2
import "."
import "../styles"

Item {
    id: root
    HifiConstants { id: hifi }
    implicitHeight: 512
    implicitWidth: 512
    property string title
    property int titleSize: titleBorder.height + 12
    property string frameColor: hifi.colors.hifiBlue
    property string backgroundColor: hifi.colors.dialogBackground
    property bool active: false

    property alias titleBorder: titleBorder
    readonly property alias titleX: titleBorder.x
    readonly property alias titleY: titleBorder.y
    readonly property alias titleWidth: titleBorder.width
    readonly property alias titleHeight: titleBorder.height

    property alias clientBorder: clientBorder
    readonly property real clientX: clientBorder.x + hifi.styles.borderWidth
    readonly property real clientY: clientBorder.y + hifi.styles.borderWidth
    readonly property real clientWidth: clientBorder.width - hifi.styles.borderWidth * 2
    readonly property real clientHeight: clientBorder.height - hifi.styles.borderWidth * 2

    /*
     * Window decorations, with a title bar and frames
     */
    Border {
        id: windowBorder
        anchors.fill: parent
        border.color: root.frameColor
        color: "#00000000"

        Border {
            id: titleBorder
            height: hifi.layout.windowTitleHeight
            anchors.right: parent.right
            anchors.left: parent.left
            border.color: root.frameColor
            clip: true
            color: root.active ?
                       hifi.colors.activeWindow.headerBackground :
                       hifi.colors.inactiveWindow.headerBackground

            Text {
                id: titleText
                color: root.active ?
                           hifi.colors.activeWindow.headerText :
                           hifi.colors.inactiveWindow.headerText
                text: root.title
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        } // header border

        Border {
            id: clientBorder
            border.color: root.frameColor
            color: root.backgroundColor
            anchors.bottom: parent.bottom
            anchors.top: titleBorder.bottom
            anchors.topMargin: -titleBorder.border.width
            anchors.right: parent.right
            anchors.left: parent.left
            clip: true
        } // client border
    } // window border

}
