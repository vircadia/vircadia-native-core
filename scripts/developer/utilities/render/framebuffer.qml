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
import "configSlider"

Column {
    spacing: 8
    Column {
        id: debug
        property var config: Render.getConfig("DebugDeferredBuffer")

        function setDebugMode(mode) {
            debug.config.enabled = (mode != -1);
            debug.config.mode = mode;
        }

        ComboBox {
            currentIndex: 0
            model: ListModel {
                id: cbItems
                ListElement { text: "Off"; color: "Yellow" }
                ListElement { text: "Depth"; color: "Green" }
                ListElement { text: "Albedo"; color: "Yellow" }
                ListElement { text: "Normal"; color: "White" }
                ListElement { text: "Roughness"; color: "White" }
                ListElement { text: "Metallic"; color: "White" }
                ListElement { text: "Emissive"; color: "White" }
                ListElement { text: "Unlit"; color: "White" }
                ListElement { text: "Occlusion"; color: "White" }
                ListElement { text: "Lightmap"; color: "White" }
                ListElement { text: "Scattering"; color: "White" }
                ListElement { text: "Lighting"; color: "White" }
                ListElement { text: "Shadow"; color: "White" }
                ListElement { text: "Linear Depth"; color: "White" }
                ListElement { text: "Mid Curvature"; color: "White" }
                ListElement { text: "Mid Normal"; color: "White" }
                ListElement { text: "Low Curvature"; color: "White" }
                ListElement { text: "Low Normal"; color: "White" }
                ListElement { text: "Debug Scattering"; color: "White" }
                ListElement { text: "Ambient Occlusion"; color: "White" }
                ListElement { text: "Ambient Occlusion Blurred"; color: "White" }
                ListElement { text: "Custom"; color: "White" }
            }
            width: 200
            onCurrentIndexChanged: { debug.setDebugMode(currentIndex - 1) }
        }
    }
}
