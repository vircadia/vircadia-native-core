import QtQuick 2.3
import QtQuick.Controls 1.2
import "."
import "../styles"

/*
 * FIXME Need to create a client property here so that objects can be
 * placed in it without having to think about positioning within the outer 
 * window.
 * 
 * Examine the QML ApplicationWindow.qml source for how it does this
 * 
 */
Item {
    id: root

    HifiPalette { id: hifiPalette }
    SystemPalette { id: sysPalette; colorGroup: SystemPalette.Active }
    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0

    property int animationDuration: 400
    property bool destroyOnInvisible: false
    property bool destroyOnCloseButton: true
    property bool resizable: false
    property int minX: 256
    property int minY: 256
    property int topMargin: root.height - clientBorder.height + 8
    property int margins: 8
    property string title
    property int titleSize: titleBorder.height + 12
    property string frameColor: hifiPalette.hifiBlue
    property string backgroundColor: sysPalette.window
    property string headerBackgroundColor: sysPalette.dark
    clip: true

    /*
     * Support for animating the dialog in and out. 
     */
    enabled: false
    scale: 0.0
    
    // The offscreen UI will enable an object, rather than manipulating it's 
    // visibility, so that we can do animations in both directions.  Because 
    // visibility and enabled are boolean flags, they cannot be animated.  So when 
    // enabled is change, we modify a property that can be animated, like scale or 
    // opacity.  
    onEnabledChanged: {
        scale = enabled ? 1.0 : 0.0
    }

    // The actual animator
    Behavior on scale {
        NumberAnimation {
            duration: root.animationDuration
            easing.type: Easing.InOutBounce
        }
    }

    // We remove any load the dialog might have on the QML by toggling it's 
    // visibility based on the state of the animated property
    onScaleChanged: {
        visible = (scale != 0.0);
    }
    
    // Some dialogs should be destroyed when they become invisible, so handle that
    onVisibleChanged: {
        if (!visible && destroyOnInvisible) {
            destroy();
        }
    }

    // our close function performs the same way as the OffscreenUI class:
    // don't do anything but manipulate the enabled flag and let the other 
    // mechanisms decide if the window should be destoryed after the close
    // animation completes
    function close() {
        if (destroyOnCloseButton) {
            destroyOnInvisible = true
        }
        enabled = false;
    }
    
    /*
     * Resize support
     */
    function deltaSize(dx, dy) {
        width = Math.max(width + dx, minX) 
        height = Math.max(height + dy, minY) 
    }

    MouseArea {
        id: sizeDrag
        property int startX
        property int startY
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 16
        height: 16
        hoverEnabled: true
        onPressed: {
            startX = mouseX
            startY = mouseY
        }
        onPositionChanged: {
            if (pressed && root.resizable) {
                root.deltaSize((mouseX - startX), (mouseY - startY))
                startX = mouseX
                startY = mouseY
            }
        }
    }

    /* 
     * Window decorations, with a title bar and frames
     */
    Border {
        id: windowBorder
        anchors.fill: parent
        border.color: root.frameColor
        color: root.backgroundColor

        Border {
            id: titleBorder
            height: 48
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            border.color: root.frameColor
            color: root.headerBackgroundColor

            Text {
                id: titleText
                // FIXME move all constant colors to our own palette class HifiPalette
                color: "white"
                text: root.title
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }

            MouseArea {
                id: titleDrag
                anchors.right: closeButton.left
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.rightMargin: 4
                drag {
                    target: root
                    minimumX: 0
                    minimumY: 0
                    maximumX: root.parent ? root.parent.width - root.width : 0
                    maximumY: root.parent ? root.parent.height - root.height : 0
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
                source: "../../styles/close.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.close();
                    }
                }
            }
        } // header border

        Border {
            id: clientBorder
            border.color: root.frameColor
            // FIXME move all constant colors to our own palette class HifiPalette
            color: "#00000000"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: titleBorder.bottom
            anchors.topMargin: -titleBorder.border.width
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            clip: true
        } // client border
    } // window border

}
