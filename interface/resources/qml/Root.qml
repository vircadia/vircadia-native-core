import Hifi 1.0
import QtQuick 2.3

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    anchors.fill: parent

    onParentChanged: {
        forceActiveFocus();
    }
}

