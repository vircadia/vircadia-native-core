import QtQuick 2.5
import Qt.labs.settings 1.0

import "../../dialogs"

PreferencesDialog {
    id: root
    objectName: "NetworkingPreferencesDialog"
    title: "Networking Settings"
    showCategories: ["Networking"]
    property var settings: Settings {
        category: root.objectName
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }
}

