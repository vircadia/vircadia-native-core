import QtQuick 2.5
import QtQuick.Controls 1.4

import "Global.js" as Global

// This is our primary 'desktop' object to which all VR dialogs and 
// windows will be childed. 
Item {
    id: desktop
    anchors.fill: parent;
    onParentChanged: forceActiveFocus();

    // Allows QML/JS to find the desktop through the parent chain
    property bool desktopRoot: true

    // The VR version of the primary menu
    property var rootMenu: Menu { objectName: "rootMenu" }

    // List of all top level windows
    property var windows: [];
    onChildrenChanged:  windows = Global.getTopLevelWindows(desktop);

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    function raise(item) {
        Global.raiseWindow(item);
    }

}
