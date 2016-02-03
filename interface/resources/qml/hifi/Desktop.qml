import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1;

import "../desktop"
import ".."

Desktop {
    id: desktop

    Component.onCompleted: {
        WebEngine.settings.javascriptCanOpenWindows = false;
        WebEngine.settings.javascriptCanAccessClipboard = false;
        WebEngine.settings.spatialNavigationEnabled = true;
        WebEngine.settings.localContentCanAccessRemoteUrls = true;
    }

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    Action {
        text: "Open Browser"
        shortcut: "Ctrl+B"
        onTriggered: {
            console.log("Open browser");
            browserBuilder.createObject(desktop);
        }
        property var browserBuilder: Component {
            Browser{}
        }
    }

}



