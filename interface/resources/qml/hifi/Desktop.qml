import QtQuick 2.5

import "../desktop"
import ".."

Desktop {
    id: desktop

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }
}



