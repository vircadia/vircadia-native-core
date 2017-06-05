import QtQuick 2.5
import Qt.labs.settings 1.0

import "../../dialogs"

PreferencesDialog {
    id: root
    objectName: "AudioBuffersDialog"
    title: "Audio Settings"
    showCategories: ["Audio Buffers"]
    property var settings: Settings {
        category: root.objectName
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }
}
