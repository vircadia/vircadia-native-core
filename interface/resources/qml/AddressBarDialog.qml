import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.2
import "controls"
import "styles"

Item {
    id: root
    HifiConstants { id: hifi }

    objectName: "AddressBarDialog"

    property int animationDuration: hifi.effects.fadeInDuration
    property bool destroyOnInvisible: false
    property bool destroyOnCloseButton: true

    implicitWidth: addressBarDialog.implicitWidth
    implicitHeight: addressBarDialog.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0

    AddressBarDialog {
        id: addressBarDialog

        property int iconOverlap: 15  // Let the circle overlap window edges and rectangular part of dialog
        property int maximumX: root.parent.width - root.width
        property int maximumY: root.parent.height - root.height

        implicitWidth: box.width + icon.width - iconOverlap * 2
        implicitHeight: addressLine.height + hifi.layout.spacing * 2

        Border {
			id: box

            width: 512
            height: parent.height
            border.width: 0
            radius: 6
            color: "#ededee"

            x: icon.width - addressBarDialog.iconOverlap * 2  // W.r.t. addressBarDialog

			MouseArea {
				id: boxDrag

                anchors.fill: parent

                drag {
					target: root
                    minimumX: 0
                    minimumY: 0
                    maximumX: root.parent ? addressBarDialog.maximumX : 0
                    maximumY: root.parent ? addressBarDialog.maximumY : 0
                }
			}

            TextInput {
				id: addressLine

                anchors.fill: parent
                anchors.leftMargin: addressBarDialog.iconOverlap + hifi.layout.spacing * 2
                anchors.rightMargin: hifi.layout.spacing * 2
                anchors.topMargin: hifi.layout.spacing
                anchors.bottomMargin: hifi.layout.spacing

                font.pointSize: 15
                helperText: "Go to: place, @user, /path, network address"

                onAccepted: {
					event.accepted
					addressBarDialog.loadAddress(addressLine.text)
				}
			}
		}

        Image {
            id: icon
            source: "../images/address-bar-icon.svg"
            width: 80
            height: 80
            anchors.right: box.left
            anchors.rightMargin: -addressBarDialog.iconOverlap
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                id: iconDrag

                anchors.fill: parent

                drag {
                    target: root
                    minimumX: 0
                    minimumY: 0
                    maximumX: root.parent ? addressBarDialog.maximumX : 0
                    maximumY: root.parent ? addressBarDialog.maximumY : 0
                }
            }
        }
    }

    // The UI enables an object, rather than manipulating its visibility, so that we can do animations in both directions. 
	// Because visibility and enabled are booleans, they cannot be animated. So when enabled is changed, we modify a property 
	// that can be animated, like scale or opacity, and then when the target animation value is reached, we can modify the 
	// visibility.
    enabled: false
    opacity: 1.0

    onEnabledChanged: {
        opacity = enabled ? 1.0 : 0.0
        if (enabled) {
            addressLine.forceActiveFocus();
        }
    }

    Behavior on opacity {
        // Animate opacity.
        NumberAnimation {
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    onOpacityChanged: {
	    // Once we're transparent, disable the dialog's visibility.
        visible = (opacity != 0.0)
    }

    onVisibleChanged: {
        if (!visible) {
            reset()

			// Some dialogs should be destroyed when they become invisible.
			if (destroyOnInvisible) {
				destroy()
			}
        }

    }

    function close() {
	    // The close function performs the same way as the OffscreenUI class: don't do anything but manipulate the enabled flag 
		// and let the other mechanisms decide if the window should be destroyed after the close animation completes.
        if (destroyOnCloseButton) {
            destroyOnInvisible = true
        }
        enabled = false
    }

    function reset() {
        addressLine.text = ""
    }

    function toggleOrGo() {
        if (addressLine.text == "") {
            enabled = false
        } else {
            addressBarDialog.loadAddress(addressLine.text)
        }
    }

    Keys.onEscapePressed: {
        enabled = false
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (event.modifiers == Qt.ControlModifier) {
                    event.accepted = true
                    enabled = false
                }
                break
        }
    }

    Keys.onReturnPressed: toggleOrGo()
    Keys.onEnterPressed: toggleOrGo()
}
