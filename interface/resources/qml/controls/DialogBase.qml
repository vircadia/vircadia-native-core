import QtQuick 2.3
import QtQuick.Controls 1.2
import "."
import "../styles"

Item {
    id: root
    objectName: "topLevelWindow"
    HifiConstants { id: hifi }
    implicitHeight: contentImplicitHeight + titleBorder.height + hifi.styles.borderWidth
    implicitWidth: contentImplicitWidth + hifi.styles.borderWidth * 2
    property string title
    property int titleSize: titleBorder.height + 12
    property string frameColor: hifi.colors.hifiBlue
    property string backgroundColor: hifi.colors.dialogBackground
    property bool active: false
    property real contentImplicitWidth: 800
    property real contentImplicitHeight: 800

    property alias titleBorder: titleBorder
    readonly property alias titleX: titleBorder.x
    readonly property alias titleY: titleBorder.y
    readonly property alias titleWidth: titleBorder.width
    readonly property alias titleHeight: titleBorder.height

    // readonly property real borderWidth: hifi.styles.borderWidth
    readonly property real borderWidth: 0
    property alias clientBorder: clientBorder
    readonly property real clientX: clientBorder.x + borderWidth
    readonly property real clientY: clientBorder.y + borderWidth
    readonly property real clientWidth: clientBorder.width - borderWidth * 2
    readonly property real clientHeight: clientBorder.height - borderWidth * 2

    /*
     * Window decorations, with a title bar and frames
     */
    Border {
        id: windowBorder
        anchors.fill: parent
        border.color: root.frameColor
        border.width: 0
        color: "#00000000"

        Border {
            id: titleBorder
            height: hifi.layout.windowTitleHeight
            anchors.right: parent.right
            anchors.left: parent.left
            border.color: root.frameColor
            border.width: 0
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

        // These two rectangles hide the curves between the title area 
        // and the client area 
        Rectangle {
            y: titleBorder.height - titleBorder.radius
            height: titleBorder.radius
            width: titleBorder.width
            color: titleBorder.color
            visible: borderWidth == 0
        }
        
        Rectangle {
            y: titleBorder.height
            width: clientBorder.width
            height: clientBorder.radius 
            color: clientBorder.color
        }

        Border {
            id: clientBorder
            border.width: 0
            border.color: root.frameColor
            color: root.backgroundColor
            anchors.bottom: parent.bottom
            anchors.top: titleBorder.bottom
            anchors.topMargin: -titleBorder.border.width
            anchors.right: parent.right
            anchors.left: parent.left
        } // client border
    } // window border

}
