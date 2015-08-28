import Hifi 1.0
import QtQuick 2.3

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    anchors.fill: parent
    signal unhandledClick ();

    // Detects a mouseclick that is not handled by any child components.
    // Used by AddressBarDialog.qml to close when user clicks outside of it.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            unhandledClick();
        }
    }

    onParentChanged: {
        forceActiveFocus();
    }
}

