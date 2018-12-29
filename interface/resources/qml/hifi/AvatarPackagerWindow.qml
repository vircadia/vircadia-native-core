import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../controlsUit" 1.0 as HifiControls
import "../stylesUit" 1.0
import "../windows" as Windows
import "../controls" 1.0
import "../dialogs"
import "avatarPackager" 1.0
import "avatarapp" 1.0 as AvatarApp

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
