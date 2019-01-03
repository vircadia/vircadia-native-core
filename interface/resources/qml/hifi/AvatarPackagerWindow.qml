import QtQuick 2.6
import "../stylesUit" 1.0
import "../windows" as Windows
import "avatarPackager" 1.0

Windows.ScrollingWindow {
    id: root
    objectName: "AvatarPackager"
    width: 480
    height: 706
    title: "Avatar Packager"
    resizable: false
    opacity: parent.opacity
    destroyOnHidden: true
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(480, 706)

    HifiConstants { id: hifi }

    AvatarPackagerApp {
        height: pane.height
        width: pane.width
    }
}
