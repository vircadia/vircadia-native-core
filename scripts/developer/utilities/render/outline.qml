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
import "configSlider"

Item {
    id: root
    property var debugConfig: Render.getConfig("RenderMainView.OutlineDebug")
    property var drawConfig: Render.getConfig("RenderMainView.OutlineEffect0")
    signal sendToScript(var message);

    Column {
        spacing: 8

        ComboBox {
            id: groupBox
            model: ["Group 0", "Group 1", "Group 2", "Group 3", "Group 4"]
            Timer {
                id: postpone
                interval: 100; running: false; repeat: false
                onTriggered: { 
                    paramWidgetLoader.sourceComponent = paramWidgets;
                    sendToScript(currentIndex)
                }
            }
            onCurrentIndexChanged: {
                // This is a hack to be sure the widgets below properly reflect the change of index: delete the Component
                // by setting the loader source to Null and then recreate it 100ms later
                root.drawConfig = Render.getConfig("RenderMainView.OutlineEffect"+currentIndex)
                paramWidgetLoader.sourceComponent = undefined;
                postpone.interval = 100
                postpone.start()
            }
        }

        CheckBox {
            text: "View Mask"
            checked: root.debugConfig["viewMask"]
            onCheckedChanged: {
                root.debugConfig["viewMask"] = checked;
            }
        }

        Component {
            id: paramWidgets
            Column {
                spacing: 8
                CheckBox {
                    text: "Glow"
                    checked: root.drawConfig["glow"]
                    onCheckedChanged: {
                        drawConfig["glow"] = checked;
                    }
                }
                ConfigSlider {
                    label: "Width"
                    integral: false
                    config: root.drawConfig
                    property: "width"
                    max: 15.0
                    min: 0.0
                    width: 280
                }  
                ConfigSlider {
                    label: "Intensity"
                    integral: false
                    config: root.drawConfig
                    property: "intensity"
                    max: 1.0
                    min: 0.0
                    width: 280
                }  

                GroupBox {
                    title: "Color"
                    width: 280
                    Column {
                        spacing: 8

                        ConfigSlider {
                            label: "Red"
                            integral: false
                            config: root.drawConfig
                            property: "colorR"
                            max: 1.0
                            min: 0.0
                            width: 270
                        }  
                        ConfigSlider {
                            label: "Green"
                            integral: false
                            config: root.drawConfig
                            property: "colorG"
                            max: 1.0
                            min: 0.0
                            width: 270
                        }  
                        ConfigSlider {
                            label: "Blue"
                            integral: false
                            config: root.drawConfig
                            property: "colorB"
                            max: 1.0
                            min: 0.0
                            width: 270
                        }
                    }
                }

                GroupBox {
                    title: "Fill Opacity"
                    width: 280
                    Column {
                        spacing: 8

                        ConfigSlider {
                            label: "Unoccluded"
                            integral: false
                            config: root.drawConfig
                            property: "unoccludedFillOpacity"
                            max: 1.0
                            min: 0.0
                            width: 270
                        }  
                        ConfigSlider {
                            label: "Occluded"
                            integral: false
                            config: root.drawConfig
                            property: "occludedFillOpacity"
                            max: 1.0
                            min: 0.0
                            width: 270
                        }
                    }
                }
            }
        }

        Loader {
            id: paramWidgetLoader
            sourceComponent: paramWidgets
        }
    }
}
