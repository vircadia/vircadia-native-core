//
//  debugWindow.qml
//
//  Brad Hefta-Gaub, created on 12/19/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Hifi 1.0 as Hifi


Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100

    property var channel;

    TextArea {
        id: textArea
        width: parent.width
        height: parent.height
        text:""
    }

    function fromScript(message) {
        var MAX_LINE_COUNT = 2000;
        var TRIM_LINES = 500;
        if (textArea.lineCount > MAX_LINE_COUNT) {
            var lines = textArea.text.split('\n');
            lines.splice(0, TRIM_LINES);
            textArea.text = lines.join('\n');
        }
        textArea.append(message);
    }
}


