import QtQuick 2.5
import Qt.labs.settings 1.0

import "../../dialogs"
import "../"

PreferencesDialog {
    id: root
    objectName: "AudioDialog"
    title: "Audio Settings"
    showFooter: false
    property var settings: Settings {
        category: root.objectName
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    Audio {}
}
