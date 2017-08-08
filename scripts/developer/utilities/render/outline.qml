//
//  outline.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: root
    property var pickConfig: Render.getConfig("RenderMainView.PickOutlined")
    property var debugConfig: Render.getConfig("RenderMainView.DebugOutline")

    Column {
        spacing: 8

        CheckBox {
            text: "Edit Outline"
            checked: root.pickConfig["enabled"]
            onCheckedChanged: {
                root.pickConfig["enabled"] = checked;
            }
        }
        CheckBox {
            text: "View Outlined Depth"
            checked: root.debugConfig["viewOutlinedDepth"]
            onCheckedChanged: {
                root.debugConfig["viewOutlinedDepth"] = checked;
            }
        }
    }
}
