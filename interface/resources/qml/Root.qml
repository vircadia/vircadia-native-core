import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4

import "Global.js" as Global

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    objectName: Global.OFFSCREEN_ROOT_OBJECT_NAME
    anchors.fill: parent;
    property var windows: [];
    property var rootMenu: Menu {
        objectName: "rootMenu"
    }

    onChildrenChanged:  {
        windows = Global.getTopLevelWindows(root);
    }

    onParentChanged: {
        forceActiveFocus();
    }
}

