//
//  FileDialog.qml
//
//  Created by Anthony Thibault on 31 Oct 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtGraphicalEffects 1.0
import "."

Rectangle {
    id: keyboardBase
    objectName: "keyboard"

    anchors.left: parent.left
    anchors.right: parent.right

    color: "#252525"

    property bool raised: false
    property bool numeric: false

    readonly property int keyboardRowHeight: 50
    readonly property int keyboardWidth: 480
    readonly property int keyboardHeight: 200

    readonly property int mirrorTextHeight: keyboardRowHeight

    property bool password: false
    property alias mirroredText: mirrorText.text
    property bool showMirrorText: true

    readonly property int raisedHeight: keyboardHeight + (showMirrorText ? keyboardRowHeight : 0)

    height: 0
    visible: false

    property bool shiftMode: false
    property bool numericShiftMode: false


    onPasswordChanged: {
        var use3DKeyboard = (typeof MenuInterface === "undefined") ? false : MenuInterface.isOptionChecked("Use 3D Keyboard");
        if (use3DKeyboard) {
            KeyboardScriptingInterface.password = password;
        }
    }

    onRaisedChanged: {
        var use3DKeyboard = (typeof MenuInterface === "undefined") ? false : MenuInterface.isOptionChecked("Use 3D Keyboard");
        if (!use3DKeyboard) {
            keyboardBase.height = raised ? raisedHeight : 0;
            keyboardBase.visible = raised;
        } else {
            KeyboardScriptingInterface.raised = raised;
            KeyboardScriptingInterface.password = raised ? password : false;
        }
        mirroredText = "";
    }

    function resetShiftMode(mode) {
        shiftMode = mode;
        shiftKey.resetToggledMode(mode);
    }

    function toUpper(str) {
        if (str === ",") {
            return "<";
        } else if (str === ".") {
            return ">";
        } else if (str === "/") {
            return "?";
        } else if (str === "-") {
            return "_";
        } else {
            return str.toUpperCase(str);
        }
    }

    function toLower(str) {
        if (str === "<") {
            return ",";
        } else if (str === ">") {
            return ".";
        } else if (str === "?") {
            return "/";
        } else if (str === "_") {
            return "-";
        } else {
            return str.toLowerCase(str);
        }
    }

    function forEachKey(func) {
        var i, j;
        for (i = 0; i < columnAlpha.children.length; i++) {
            var row = columnAlpha.children[i];
            for (j = 0; j < row.children.length; j++) {
                var key = row.children[j];
                func(key);
            }
        }
    }

    onShiftModeChanged: {
        forEachKey(function (key) {
            if (/[a-z-_]/i.test(key.glyph)) {
                if (shiftMode) {
                    key.glyph = keyboardBase.toUpper(key.glyph);
                } else {
                    key.glyph = keyboardBase.toLower(key.glyph);
                }
            }
        });
    }

    function alphaKeyClickedHandler(mouseArea) {
        // reset shift mode to false after first keypress
        if (shiftMode) {
            resetShiftMode(false);
        }
    }

    Component.onCompleted: {
        // hook up callbacks to every ascii key
        forEachKey(function (key) {
            if (/^[a-z]+$/i.test(key.glyph)) {
                key.mouseArea.onClicked.connect(alphaKeyClickedHandler);
            }
        });
    }

    Rectangle {
        height: showMirrorText ? mirrorTextHeight : 0
        width: keyboardWidth
        color: "#252525"
        anchors.horizontalCenter: parent.horizontalCenter

        TextInput {
            id: mirrorText
            visible: showMirrorText
            font.family: "Fira Sans"
            font.pixelSize: 20
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: "#00B4EF";
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            wrapMode: Text.WordWrap
            readOnly: false // we need this to allow control to accept QKeyEvent
            selectByMouse: false
            echoMode: password ? TextInput.Password : TextInput.Normal

            Keys.onPressed: {
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Space) {
                    mirrorText.text = "";
                    event.accepted = true;
                }
            }

            MouseArea { // ... and we need this mouse area to prevent mirrorText from getting mouse events to ensure it will never get focus
                anchors.fill: parent
            }
        }
    }

    Rectangle {
        id: keyboardRect
        y: showMirrorText ? mirrorTextHeight : 0
        width: keyboardWidth
        height: keyboardHeight
        color: "#252525"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        Column {
            id: columnAlpha
            width: keyboardWidth
            height: keyboardHeight
            visible: !numeric

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key { width: 43; glyph: "q"; }
                Key { width: 43; glyph: "w"; }
                Key { width: 43; glyph: "e"; }
                Key { width: 43; glyph: "r"; }
                Key { width: 43; glyph: "t"; }
                Key { width: 43; glyph: "y"; }
                Key { width: 43; glyph: "u"; }
                Key { width: 43; glyph: "i"; }
                Key { width: 43; glyph: "o"; }
                Key { width: 43; glyph: "p"; }
                Key { width: 43; glyph: "←"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 20

                Key { width: 43; glyph: "a"; }
                Key { width: 43; glyph: "s"; }
                Key { width: 43; glyph: "d"; }
                Key { width: 43; glyph: "f"; }
                Key { width: 43; glyph: "g"; }
                Key { width: 43; glyph: "h"; }
                Key { width: 43; glyph: "j"; }
                Key { width: 43; glyph: "k"; }
                Key { width: 43; glyph: "l"; }
                Key { width: 70; glyph: "⏎"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    id: shiftKey
                    width: 43
                    glyph: "⇪"
                    toggle: true
                    onToggledChanged: shiftMode = toggled
                }
                Key { width: 43; glyph: "z"; }
                Key { width: 43; glyph: "x"; }
                Key { width: 43; glyph: "c"; }
                Key { width: 43; glyph: "v"; }
                Key { width: 43; glyph: "b"; }
                Key { width: 43; glyph: "n"; }
                Key { width: 43; glyph: "m"; }
                Key { width: 43; glyph: "-"; }
                Key { width: 43; glyph: "/"; }
                Key { width: 43; glyph: "?"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    width: 70
                    glyph: "123"
                    mouseArea.onClicked: keyboardBase.parent.punctuationMode = true
                }
                Key { width: 231; glyph: " "; }
                Key { width: 43; glyph: ","; }
                Key { width: 43; glyph: "."; }
                Key {
                    fontFamily: "hifi-glyphs";
                    fontPixelSize: 48;
                    letterAnchors.topMargin: -4;
                    verticalAlignment: Text.AlignVCenter;
                    width: 86; glyph: "\ue02b";
                }
            }
        }

        Column {
            id: columnNumeric
            width: keyboardWidth
            height: keyboardHeight
            visible: numeric

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key { width: 43; glyph: "1"; }
                Key { width: 43; glyph: "2"; }
                Key { width: 43; glyph: "3"; }
                Key { width: 43; glyph: "4"; }
                Key { width: 43; glyph: "5"; }
                Key { width: 43; glyph: "6"; }
                Key { width: 43; glyph: "7"; }
                Key { width: 43; glyph: "8"; }
                Key { width: 43; glyph: "9"; }
                Key { width: 43; glyph: "0"; }
                Key { width: 43; glyph: "←"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key { width: 43; glyph: "!"; }
                Key { width: 43; glyph: "@"; }
                Key { width: 43; glyph: "#"; }
                Key { width: 43; glyph: "$"; }
                Key { width: 43; glyph: "%"; }
                Key { width: 43; glyph: "^"; }
                Key { width: 43; glyph: "&"; }
                Key { width: 43; glyph: "*"; }
                Key { width: 43; glyph: "("; }
                Key { width: 43; glyph: ")"; }
                Key { width: 43; glyph: "⏎"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    id: numericShiftKey
                    width: 43
                    glyph: "\u21E8"
                    toggle: true
                    onToggledChanged: numericShiftMode = toggled
                }
                Key { width: 43; glyph: numericShiftMode ? "`" : "+"; }
                Key { width: 43; glyph: numericShiftMode ? "~" : "-"; }
                Key { width: 43; glyph: numericShiftMode ? "\u00A3" : "="; }
                Key { width: 43; glyph: numericShiftMode ? "\u20AC" : ";"; }
                Key { width: 43; glyph: numericShiftMode ? "\u00A5" : ":"; }
                Key { width: 43; glyph: numericShiftMode ? "<" : "'"; }
                Key { width: 43; glyph: numericShiftMode ? ">" : "\""; }
                Key { width: 43; glyph: numericShiftMode ? "[" : "{"; }
                Key { width: 43; glyph: numericShiftMode ? "]" : "}"; }
                Key { width: 43; glyph: numericShiftMode ? "\\" : "|"; }
            }

            Row {
                width: keyboardWidth
                height: keyboardRowHeight
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    width: 70
                    glyph: "abc"
                    mouseArea.onClicked: keyboardBase.parent.punctuationMode = false
                }
                Key { width: 231; glyph: " "; }
                Key { width: 43; glyph: ","; }
                Key { width: 43; glyph: "."; }
                Key {
                    fontFamily: "hifi-glyphs";
                    fontPixelSize: 48;
                    letterAnchors.topMargin: -4;
                    verticalAlignment: Text.AlignVCenter;
                    width: 86; glyph: "\ue02b";
                }
            }
        }
    }
}
