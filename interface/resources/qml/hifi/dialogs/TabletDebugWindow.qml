//
//  TabletDebugWindow.qml
//
//  Vlad Stelmahovsky, created on 20/03/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Hifi 1.0 as Hifi

import "../../styles-uit"
import "../../controls-uit" as HifiControls

Rectangle {
    id: root

    objectName: "DebugWindow"

    property var title: "Debug Window"
    property bool isHMD: false

    color: hifi.colors.baseGray

    HifiConstants { id: hifi }

    signal sendToScript(var message);
    property int colorScheme: hifi.colorSchemes.dark

    property var channel;
    property var scripts: ScriptDiscoveryService;

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

    function getFormattedDate() {
        var date = new Date();
        return date.getMonth() + "/" + date.getDate() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
    }

    function sendToLogWindow(type, message, scriptFileName) {
        var typeFormatted = "";
        if (type) {
            typeFormatted = type + " - ";
        }
        fromScript("[" + getFormattedDate() + "] " + "[" + scriptFileName + "] " + typeFormatted + message);
    }

    Connections {
        target: ScriptDiscoveryService
        onPrintedMessage: sendToLogWindow("", message, engineName);
        onWarningMessage: sendToLogWindow("WARNING", message, engineName);
        onErrorMessage: sendToLogWindow("ERROR", message, engineName);
        onInfoMessage: sendToLogWindow("INFO", message, engineName);
    }

    TextArea {
        id: textArea
        width: parent.width
        height: parent.height
        backgroundVisible: false
        textColor: hifi.colors.white
        text:""
    }

}
