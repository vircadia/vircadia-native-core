import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.3
import "hifiConstants.js" as HifiConstants

Item {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    id: dialog
    width: 256
    height: 256
    scale: 0.0
    enabled: false
    property int animationDuration: 400
    property bool destroyOnInvisible: false
    property bool destroyOnCloseButton: true
    
    onEnabledChanged: {
        scale = enabled ? 1.0 : 0.0
    }
    
    onScaleChanged: {
        visible = (scale != 0.0);
    }
    
    onVisibleChanged: {
        if (!visible && destroyOnInvisible) {
            console.log("Destroying closed component");
            destroy();
        }
    }

    function close() {
        if (destroyOnCloseButton) {
            destroyOnInvisible = true
        }
        enabled = false;
    }

    Behavior on scale {
        NumberAnimation {
            //This specifies how long the animation takes
            duration: dialog.animationDuration
            //This selects an easing curve to interpolate with, the default is Easing.Linear
            easing.type: Easing.InOutBounce
        }
    }

    property int topMargin: dialog.height - clientBorder.height + 8
    property int margins: 8
    property string title
    property int titleSize: titleBorder.height + 12
    property string frameColor: HifiConstants.color
    property string backgroundColor: myPalette.window
    property string headerBackgroundColor: myPalette.dark

    CustomBorder {
        id: windowBorder
        anchors.fill: parent
        border.color: dialog.frameColor
        color: dialog.backgroundColor

        CustomBorder {
            id: titleBorder
            height: 48
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            border.color: dialog.frameColor
            color: dialog.headerBackgroundColor

            CustomText {
                id: titleText
                color: "white"
                text: dialog.title
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }

            MouseArea {
                id: titleDrag
                property int startX
                property int startY
                anchors.right: closeButton.left
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.rightMargin: 4
                drag {
                    target: dialog
                    minimumX: 0
                    minimumY: 0
                    maximumX: dialog.parent ? dialog.parent.width - dialog.width : 0
                    maximumY: dialog.parent ? dialog.parent.height - dialog.height : 0
                }
            }
            Image {
                id: closeButton
                x: 360
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                anchors.right: parent.right
                anchors.rightMargin: 12
                source: "../styles/close.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        dialog.close();
                    }
                }
            }
        } // header border

        CustomBorder {
            id: clientBorder
            border.color: dialog.frameColor
            color: "#00000000"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: titleBorder.bottom
            anchors.topMargin: -titleBorder.border.width
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
        } // client border
    } // window border
}
