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

Item {
    // Items
    ItemsSlider {
        y: 0 * 25
        label: qsTr("Opaque")
        config: Render.getConfig("DrawOpaqueDeferred")
    }
    ItemsSlider {
        y: 1 * 25
        label: qsTr("Transparent")
        config: Render.getConfig("DrawTransparentDeferred")
    }
    ItemsSlider {
        y: 2 * 25
        label: qsTr("Overlay3D")
        config: Render.getConfig("DrawOverlay3D")
    }

    // Draw status
    Item {
        y: 100

        CheckBox {
            text: qsTr("Display Status")
            partiallyCheckedEnabled: false
            onCheckedChanged: { Render.getConfig("DrawStatus").showDisplay = checked }
        }
        CheckBox {
            x: 200
            text: qsTr("Network/Physics Status")
            partiallyCheckedEnabled: false
            onCheckedChanged: { Render.getConfig("DrawStatus").showNetwork = checked }
        }
    }

    // Tone mapping
    ConfigSlider {
        y: 125
        config: Render.getConfig("ToneMapping")
        prop: "exposure"
        label: qsTr("Tone Mapping Exposure")
        min: -10; max: 10
    }

    // Ambient occlusion
    AO { y: 175 }

    // Debug buffer
    Buffer { y: 475 }
}

