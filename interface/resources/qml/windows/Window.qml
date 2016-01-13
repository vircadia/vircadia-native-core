import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import "."
import "../styles"

FocusScope {
    id: window
    objectName: "topLevelWindow"
    HifiConstants { id: hifi }
    implicitHeight: frame.height
    implicitWidth: frame.width

    property bool topLevelWindow: true
    property string title
    property bool showFrame: true
    property bool closable: true
    property bool destroyOnInvisible: false
    property bool destroyOnCloseButton: true
    property bool pinned: false
    property bool resizable: false
    property real minX: 320
    property real minY: 240;
    default property var content
    property var frame: DefaultFrame { anchors.fill: content }
    property var blur: FastBlur { anchors.fill: content; source: content; visible: false; radius: 0}
    //property var hoverDetector: MouseArea { anchors.fill: frame; hoverEnabled: true; propagateComposedEvents: true; }
    //property bool mouseInWindow: hoverDetector.containsMouse
    children: [ frame, content, blur ]
    signal windowDestroyed();

    QtObject {
        id: d
        property vector2d minPosition: Qt.vector2d(0, 0);
        property vector2d maxPosition: Qt.vector2d(100, 100);

        function clamp(value, min, max) {
            return Math.min(Math.max(value, min), max);
        }

        function updateParentRect() {
//            if (!frame) { return; }
//            console.log(window.parent.width);
//            console.log(frame.width);
//            minPosition = Qt.vector2d(-frame.anchors.leftMargin, -frame.anchors.topMargin);
//            maxPosition = Qt.vector2d(
//                Math.max(minPosition.x, Desktop.width - frame.width + minPosition.x),
//                Math.max(minPosition.y, Desktop.height - frame.height + minPosition.y))
//            console.log(maxPosition);
        }

        function keepOnScreen() {
            //window.x = clamp(x, minPosition.x, maxPosition.x);
            //window.y = clamp(y, minPosition.y, maxPosition.y);
        }

        onMinPositionChanged: keepOnScreen();
        onMaxPositionChanged: keepOnScreen();
    }

    Component.onCompleted: {
        d.updateParentRect();
        raise();
    }

    Component.onDestruction: {
        console.log("Destroyed " + window);
        windowDestroyed();
    }

    onParentChanged: {
        d.updateParentRect();
        raise();
    }

    onFrameChanged: d.updateParentRect();
    onWidthChanged: d.updateParentRect();
    onHeightChanged: d.updateParentRect();
    onXChanged: d.keepOnScreen();
    onYChanged: d.keepOnScreen();

    Connections {
        target: frame
        onRaise: window.raise();
        onClose: window.close();
        onPin: window.pin();
        onDeltaSize: {
            console.log("deltaSize")
            content.width = Math.max(content.width + dx, minX)
            content.height = Math.max(content.height + dy, minY)
        }
    }


    function raise() {
        if (enabled && parent) {
            Desktop.raise(window)
            if (!focus) {
                focus = true;
            }
        }
    }

    function pin() {
        pinned = ! pinned
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

    onEnabledChanged: {
        if (!enabled) {
            if (blur) {
                blur.visible = true;
            }
            if (content) {
                content.visible = false;
            }
        }

        opacity = enabled ? 1.0 : 0.0
        // If the dialog is initially invisible, setting opacity doesn't
        // trigger making it visible.
        if (enabled) {
            visible = true;
            raise();
        }
    }

    // The offscreen UI will enable an object, rather than manipulating it's
    // visibility, so that we can do animations in both directions.  Because
    // visibility and enabled are boolean flags, they cannot be animated.  So when
    // enabled is change, we modify a property that can be animated, like scale or
    // opacity, and then when the target animation value is reached, we can
    // modify the visibility

    // The actual animator
    Behavior on opacity {
        NumberAnimation {
            duration: hifi.effects.fadeInDuration
            easing.type: Easing.OutCubic
        }
    }

    // Once we're transparent, disable the dialog's visibility
    onOpacityChanged: {
        visible = (opacity != 0.0);
        if (opacity == 1.0) {
            content.visible = true;
            blur.visible = false;
        }
    }

    // Some dialogs should be destroyed when they become
    // invisible, so handle that
    onVisibleChanged: {
        if (!visible && destroyOnInvisible) {
            console.log("Destroying " + window);
            destroy();
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
