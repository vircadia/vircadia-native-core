//
//  FocusHack.qml
//
//  Created by Bradley Austin Davis on 21 Jan 2015
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
