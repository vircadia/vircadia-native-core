import QtQuick 2.5
import QtQuick.Controls 1.4

import "Global.js" as Global

// This is our primary 'desktop' object to which all VR dialogs and 
// windows will be childed. 
Item {
    id: desktop
    objectName: Global.OFFSCREEN_ROOT_OBJECT_NAME
    anchors.fill: parent;
    property var windows: [];
    property var rootMenu: Menu {
        objectName: "rootMenu"
    }

    onChildrenChanged:  {
        windows = Global.getTopLevelWindows(desktop);
    }

    onParentChanged: {
        forceActiveFocus();
    }

    function raise(item) {
        Global.raiseWindow(item);
    }
}
