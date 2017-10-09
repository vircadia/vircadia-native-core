//
//  bloom.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 09/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Item {
    id: root
    property var config: Render.getConfig("RenderMainView.Bloom")
    property var configThreshold: Render.getConfig("RenderMainView.BloomThreshold")
    property var configDebug: Render.getConfig("RenderMainView.DebugBloom")

    Column {
        spacing: 8

        CheckBox {
            text: "Enable"
            checked: root.config["enabled"]
            onCheckedChanged: {
                root.config["enabled"] = checked;
            }
        }
        CheckBox {
            text: "Debug"
            checked: root.configDebug["enabled"]
            onCheckedChanged: {
                root.configDebug["enabled"] = checked;
            }
        }
        ConfigSlider {
            label: "Intensity"
            integral: false
            config: root.config
            property: "intensity"
            max: 1.0
            min: 0.0
            width: 280
        }  
        ConfigSlider {
            label: "Size"
            integral: false
            config: root.config
            property: "size"
            max: 1.0
            min: 0.0
            width: 280
        }  
        ConfigSlider {
            label: "Threshold"
            integral: false
            config: root.configThreshold
            property: "threshold"
            max: 1.0
            min: 0.0
            width: 280
        }  
    }
}
