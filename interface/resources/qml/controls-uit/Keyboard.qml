//
//  FileDialog.qml
//
//  Created by Anthony Thibault on 31 Oct 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.0

Item {
    id: keyboardBase

    property bool raised: false
    property bool numeric: false

    readonly property int raisedHeight: 200

    height: enabled && raised ? raisedHeight : 0
    visible: enabled && raised

    property bool shiftMode: false

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
            if (shiftMode) {
                key.glyph = keyboardBase.toUpper(key.glyph);
            } else {
                key.glyph = keyboardBase.toLower(key.glyph);
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
        id: leftRect
        y: 0
        height: 200
        color: "#252525"
        anchors.right: keyboardRect.left
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
    }

    Rectangle {
        id: keyboardRect
        x: 206
        y: 0
        width: 480
        height: 200
        color: "#252525"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        Column {
            id: columnAlpha
            width: 480
            height: 200
            visible: !numeric

            Row {
                width: 480
                height: 50
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
                width: 480
                height: 50
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
                id: row3
                width: 480
                height: 50
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
                Key { width: 43; glyph: "_"; }
                Key { width: 43; glyph: "?"; }
                Key { width: 43; glyph: "/"; }
            }

            Row {
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    width: 86
                    glyph: "123"
                    mouseArea.onClicked: keyboardBase.parent.punctuationMode = true
                }
                Key { width: 285; glyph: " "; }
                Key { width: 43; glyph: "⇦"; }
                Key { width: 43; glyph: "⇨"; }
            }
        }

        Column {
            id: columnNumeric
            width: 480
            height: 200
            visible: numeric

            Row {
                width: 480
                height: 50
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
                width: 480
                height: 50
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
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key { width: 43; glyph: "="; }
                Key { width: 43; glyph: "+"; }
                Key { width: 43; glyph: "-"; }
                Key { width: 43; glyph: ","; }
                Key { width: 43; glyph: "."; }
                Key { width: 43; glyph: ";"; }
                Key { width: 43; glyph: ":"; }
                Key { width: 43; glyph: "'"; }
                Key { width: 43; glyph: "\""; }
                Key { width: 43; glyph: "<"; }
                Key { width: 43; glyph: ">"; }
            }

            Row {
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    width: 86
                    glyph: "abc"
                    mouseArea.onClicked: keyboardBase.parent.punctuationMode = false
                }
                Key { width: 285; glyph: " "; }
                Key { width: 43; glyph: "⇦"; }
                Key { width: 43; glyph: "⇨"; }
            }
        }
    }

    Rectangle {
        id: rightRect
        y: 280
        height: 200
        color: "#252525"
        border.width: 0
        anchors.left: keyboardRect.right
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
    }

    Rectangle {
        id: rectangle1
        color: "#ffffff"
        anchors.bottom: keyboardRect.top
        anchors.bottomMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }
}
