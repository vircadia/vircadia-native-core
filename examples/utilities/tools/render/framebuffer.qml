//
//  main.qml
//  examples/utilities/tools/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Column {
    spacing: 8
    Column {
        id: debug
        property var config: Render.getConfig("DebugDeferredBuffer")

        function setDebugMode(mode) {
            debug.config.enabled = (mode != -1);
            debug.config.mode = mode;
        }

        Label { text: qsTr("Debug Buffer") }
        ExclusiveGroup { id: bufferGroup }
        Repeater {
            model: [
                "Off",
                "Depth",
                "Albedo",
                "Normal",
                "Roughness",
                "Metallic",     
                "Emissive",
                "Occlusion",
                "Lightmap",
                "Lighting",
                "Shadow",
                "Pyramid Depth",
                "Ambient Occlusion",
                "Custom Shader"
            ]
           RadioButton {
                text: qsTr(modelData)
                exclusiveGroup: bufferGroup
                checked: index == 0
                onCheckedChanged: if (checked) debug.setDebugMode(index - 1);
            }
        }
    }
}
