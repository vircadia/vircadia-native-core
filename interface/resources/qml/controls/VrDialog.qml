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
DialogBase {
    id: root
    HifiConstants { id: hifi }
    // FIXME better placement via a window manager
    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0

    property bool destroyOnInvisible: false
    property bool destroyOnCloseButton: true
    property bool resizable: false

    property int animationDuration: hifi.effects.fadeInDuration
    property int minX: 256
    property int minY: 256
    readonly property int topMargin: root.height - clientBorder.height + hifi.layout.spacing

    /*
     * Support for animating the dialog in and out. 
     */
    enabled: false
    opacity: 1.0
    
    // The offscreen UI will enable an object, rather than manipulating it's 
    // visibility, so that we can do animations in both directions.  Because 
    // visibility and enabled are boolean flags, they cannot be animated.  So when 
    // enabled is change, we modify a property that can be animated, like scale or 
    // opacity, and then when the target animation value is reached, we can
    // modify the visibility
    onEnabledChanged: {
        opacity = enabled ? 1.0 : 0.0
        // If the dialog is initially invisible, setting opacity doesn't 
        // trigger making it visible.
        if (enabled) {
            visible = true;
        }
    }

    // The actual animator
    Behavior on opacity {
        NumberAnimation {
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    // Once we're transparent, disable the dialog's visibility
    onOpacityChanged: {
        visible = (opacity != 0.0);
    }

    // Some dialogs should be destroyed when they become invisible,
    // so handle that
    onVisibleChanged: {
        if (!visible && destroyOnInvisible) {
            destroy();
        }
    }

    // our close function performs the same way as the OffscreenUI class:
    // don't do anything but manipulate the enabled flag and let the other 
    // mechanisms decide if the window should be destroyed after the close
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
        enabled: root.resizable
        property int startX
        property int startY
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 16
        height: 16
        z: 1000
        hoverEnabled: true
        onPressed: {
            startX = mouseX
            startY = mouseY
        }
        onPositionChanged: {
            if (pressed) {
                root.deltaSize((mouseX - startX), (mouseY - startY))
                startX = mouseX
                startY = mouseY
            }
        }
    }

    MouseArea {
        id: titleDrag
        x: root.titleX
        y: root.titleY
        width: root.titleWidth
        height: root.titleHeight

        drag {
            target: root
            minimumX: 0
            minimumY: 0
            maximumX: root.parent ? root.parent.width - root.width : 0
            maximumY: root.parent ? root.parent.height - root.height : 0
        }

        Row {
            id: windowControls
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.rightMargin: hifi.layout.spacing
            FontAwesome {
                id: icon
                anchors.verticalCenter: parent.verticalCenter
                size: root.titleHeight - hifi.layout.spacing * 2
                color: "red"
                text: "\uf00d"
                MouseArea {
                    anchors.margins: hifi.layout.spacing / 2
                    anchors.fill: parent
                    onClicked: {
                        root.close();
                    }
                }
            }
        }
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (event.modifiers == Qt.ControlModifier) {
                    event.accepted = true
                    enabled = false
                }
                break;
        }
    }
}
