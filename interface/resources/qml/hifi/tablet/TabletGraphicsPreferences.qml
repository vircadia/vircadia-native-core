import QtQuick 2.5
import Qt.labs.settings 1.0

import "tabletWindows"
import "../../dialogs"
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

StackView {
    id: profileRoot
    initialItem: root
    objectName: "stack"

    property var eventBridge;
    signal sendToScript(var message);

    function pushSource(path) {
        editRoot.push(Qt.reslovedUrl(path));
    }

    function popSource() {

    }

    TabletPreferencesDialog {
        id: root
        property string title: "Graphics Settings"
        objectName: "TabletGraphicsPreferences"
        width: parent.width
        height: parent.height
        showCategories: ["Graphics"]
    }
}
