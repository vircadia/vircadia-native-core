//
//  shadow.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 10/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Column {
    id: root
    spacing: 8
    property var config: Render.getConfig("RenderMainView.DrawFrustums");

    Component.onCompleted: {
        config.enabled = true;
    }
    Component.onDestruction: {
        config.enabled = false;
    }

    CheckBox {
        text: "Freeze Frustums"
        checked: false
        onCheckedChanged: { 
            config.isFrozen = checked;
        }
    }
    Row {
        spacing: 8
        Label {
            text: "View"
            color: "yellow"
            font.italic: true
        }
        Label {
            text: "Shadow"
            color: "red"
            font.italic: true
        }
    }
}
