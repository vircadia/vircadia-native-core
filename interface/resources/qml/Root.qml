import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    objectName: "desktopRoot"
    anchors.fill: parent
    property var rootMenu: Menu {
        objectName: "rootMenu"
    }

    onParentChanged: {
        forceActiveFocus();
    }
}

