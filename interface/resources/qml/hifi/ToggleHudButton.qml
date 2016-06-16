import QtQuick 2.5
import QtQuick.Controls 1.4

import "../windows"

Window {
    //frame: HiddenFrame {}
    hideBackground: true
    resizable: false
    destroyOnCloseButton: false
    destroyOnHidden: false
    closable: false
    shown: true
    pinned: true
    width: 50
    height: 50
    clip: true
    visible: true
    // Disable this window from being able to call 'desktop.raise() and desktop.showDesktop'
    activator: Item {}

    Item {
        width: 50
        height: 50
        Image {
            y: desktop.pinned ? -50 : 0
            id: hudToggleImage
            source: "../../icons/hud-01.svg"
        }
        MouseArea {
            readonly property string overlayMenuItem: "Overlays"
            anchors.fill: parent
            onClicked: MenuInterface.setIsOptionChecked(overlayMenuItem, !MenuInterface.isOptionChecked(overlayMenuItem));
        }
    }
}


