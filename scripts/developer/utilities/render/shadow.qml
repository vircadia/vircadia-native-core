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
    property var viewConfig: Render.getConfig("RenderMainView.DrawViewFrustum");
    property var shadowConfig: Render.getConfig("RenderMainView.DrawShadowFrustum");

    Component.onCompleted: {
        viewConfig.enabled = true;
        shadowConfig.enabled = true;
    }
    Component.onDestruction: {
        viewConfig.enabled = false;
        shadowConfig.enabled = false;
    }

    CheckBox {
        text: "Freeze Frustums"
        checked: false
        onCheckedChanged: { 
            viewConfig.isFrozen = checked;
            shadowConfig.isFrozen = checked;
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
            color: "blue"
            font.italic: true
        }
    }
}
