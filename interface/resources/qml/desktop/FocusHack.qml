import QtQuick 2.5

FocusScope {
    id: root
    objectName: "FocusHack"

    TextInput {
        id: textInput;
        focus: true
        width: 10; height: 10
        onActiveFocusChanged: root.destroy()
    }

    Timer {
        id: focusTimer
        running: false
        interval: 100
        onTriggered: textInput.forceActiveFocus()
    }

    function start() {
        focusTimer.running = true;
    }
}



