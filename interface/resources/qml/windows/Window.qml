import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import "."
import "../styles"

FocusScope {
    id: window
    HifiConstants { id: hifi }
    // The Window size is the size of the content, while the frame
    // decorations can extend outside it.  Windows should generally not be
    // given explicit height / width, but rather be allowed to conform to
    // their content
    implicitHeight: content.height
    implicitWidth: content.width

    property bool topLevelWindow: true
    property string title
    // Should the window include a close control?
    property bool closable: true
    // Should hitting the close button hide or destroy the window?
    property bool destroyOnCloseButton: true
    // Should hiding the window destroy it or just hide it?
    property bool destroyOnInvisible: false
    // FIXME support for pinned / unpinned pending full design
    // property bool pinnable: false
    // property bool pinned: false
    property bool resizable: false
    property vector2d minSize: Qt.vector2d(100, 100)
    property vector2d maxSize: Qt.vector2d(1280, 720)

    // The content to place inside the window, determined by the client
    default property var content


    onContentChanged: {
        if (content) {
            content.anchors.fill = window
        }
    }

    // Default to a standard frame.  Can be overriden to provide custom
    // frame styles, like a full desktop frame to simulate a modal window
    property var frame: DefaultFrame {
        z: -1
        anchors.fill: parent
    }

    // This mouse area serves to raise the window. To function, it must live 
    // in the window and have a higher Z-order than the content, but follow 
    // the position and size of frame decoration
    property var activator: MouseArea {
        width: frame.decoration.width
        height: frame.decoration.height
        x: frame.decoration.anchors.margins
        y: frame.decoration.anchors.topMargin
        propagateComposedEvents: true
        hoverEnabled: true
        onPressed: { window.raise(); mouse.accepted = false; }
        // Debugging visualization
        // Rectangle { anchors.fill:parent; color: "#7f00ff00" }
    }

    children: [ frame, content, activator ]
    signal windowDestroyed();

    Component.onCompleted: {
        fadeTargetProperty = visible ? 1.0 : 0.0
        raise();
    }

    Component.onDestruction: {
        content.destroy();
        console.log("Destroyed " + window);
        windowDestroyed();
    }
    onParentChanged: raise();

    Connections {
        target: frame
        onRaise: window.raise();
        onClose: window.close();
        onPin: window.pin();
        onDeltaSize: {
            var newSize = Qt.vector2d(content.width + dx, content.height + dy);
            newSize = clampVector(newSize, minSize, maxSize);
            window.width = newSize.x
            window.height = newSize.y
        }
    }

    function raise() {
        if (visible && parent) {
            Desktop.raise(window)
            if (!focus) {
                focus = true;
            }
        }
    }

    function pin() {
//        pinned = ! pinned
    }

    // our close function performs the same way as the OffscreenUI class:
    // don't do anything but manipulate the targetVisible flag and let the other
    // mechanisms decide if the window should be destroyed after the close
    // animation completes
    function close() {
        console.log("Closing " + window)
        if (destroyOnCloseButton) {
            destroyOnInvisible = true
        }
        visible = false;
    }

    //
    // Enable window visibility transitions
    //

    // The target property to animate, usually scale or opacity
    property alias fadeTargetProperty: window.opacity
    // always start the property at 0 to enable fade in on creation
    opacity: 0

    // Some dialogs should be destroyed when they become
    // invisible, so handle that
    onVisibleChanged: {
        // If someone directly set the visibility to false
        // toggle it back on and use the targetVisible flag to transition
        // via fading.
        if ((!visible && fadeTargetProperty != 0.0) || (visible && fadeTargetProperty == 0.0)) {
            var target = visible;
            visible = !visible;
            fadeTargetProperty = target ? 1.0 : 0.0;
            return;
        }
        if (!visible && destroyOnInvisible) {
            console.log("Destroying " + window);
            destroy();
            return;
        }
    }

    // The offscreen UI will enable an object, rather than manipulating it's
    // visibility, so that we can do animations in both directions.  Because
    // visibility is a boolean flags, it cannot be animated.  So when
    // targetVisible is changed, we modify a property that can be animated,
    // like scale or opacity, and then when the target animation value is reached,
    // we can modify the visibility

    // The actual animator
    Behavior on fadeTargetProperty {
        NumberAnimation {
            duration: hifi.effects.fadeInDuration
            easing.type: Easing.OutCubic
        }
    }

    // Once we're transparent, disable the dialog's visibility
    onFadeTargetPropertyChanged: {
        visible = (fadeTargetProperty != 0.0);
    }


    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (event.modifiers === Qt.ControlModifier) {
                    event.accepted = true
                    visible = false
                }
                break;
        }
    }


    function clamp(value, min, max) {
        return Math.min(Math.max(value, min), max);
    }

    function clampVector(value, min, max) {
        return Qt.vector2d(
                    clamp(value.x, min.x, max.x),
                    clamp(value.y, min.y, max.y))
    }

}
