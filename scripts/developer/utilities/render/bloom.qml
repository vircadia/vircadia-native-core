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
        GroupBox {
            title: "Debug"
            Row {
                ExclusiveGroup { id: debugGroup }
                RadioButton {
                    text : "Off"
                    checked : !root.configDebug["enabled"]
                    onCheckedChanged: {
                        if (checked) {
                            root.configDebug["enabled"] = false
                        }
                    }
                    exclusiveGroup : debugGroup
                }
                RadioButton {
                    text : "Lvl 0"
                    checked :root.configDebug["enabled"] && root.configDebug["mode"]==0
                    onCheckedChanged: {
                        if (checked) {
                            root.configDebug["enabled"] = true
                            root.configDebug["mode"] = 0
                        }
                    }
                    exclusiveGroup : debugGroup
                }
                RadioButton {
                    text : "Lvl 1"
                    checked : root.configDebug["enabled"] && root.configDebug["mode"]==1
                    onCheckedChanged: {
                        if (checked) {
                            root.configDebug["enabled"] = true
                            root.configDebug["mode"] = 1
                        }
                    }
                    exclusiveGroup : debugGroup
                }
                RadioButton {
                    text : "Lvl 2"
                    checked : root.configDebug["enabled"] && root.configDebug["mode"]==2
                    onCheckedChanged: {
                        if (checked) {
                            root.configDebug["enabled"] = true
                            root.configDebug["mode"] = 2
                        }
                    }
                    exclusiveGroup : debugGroup
                }
                RadioButton {
                    text : "All"
                    checked : root.configDebug["enabled"] && root.configDebug["mode"]==3
                    onCheckedChanged: {
                        if (checked) {
                            root.configDebug["enabled"] = true
                            root.configDebug["mode"] = 3
                        }
                    }
                    exclusiveGroup : debugGroup
                }
            }
        }
        ConfigSlider {
            label: "Intensity"
            integral: false
            config: root.config
            property: "intensity"
            max: 5.0
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
            max: 2.0
            min: 0.0
            width: 280
        }  
    }
}
