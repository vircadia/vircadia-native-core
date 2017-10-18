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
    signal sendToScript(var message);

    Column {
        spacing: 8
        anchors.fill: parent

        CheckBox {
            id: debug
            text: "View Mask"
            checked: root.debugConfig["viewMask"]
            onCheckedChanged: {
                root.debugConfig["viewMask"] = checked;
            }
        }

        TabView {
            id: tabs
            width: 384
            height: 400

            onCurrentIndexChanged: {
                sendToScript(currentIndex)
            }

            Component {
                id: paramWidgets

                Column {
                    spacing: 8

                    CheckBox {
                        id: glow
                        text: "Glow"
                        checked: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)["glow"]
                        onCheckedChanged: {
                            Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)["glow"] = checked;
                        }
                    }
                    ConfigSlider {
                        label: "Width"
                        integral: false
                        config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
                        property: "width"
                        max: 15.0
                        min: 0.0
                        width: 280
                    }  
                    ConfigSlider {
                        label: "Intensity"
                        integral: false
                        config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
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
                                config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
                                property: "colorR"
                                max: 1.0
                                min: 0.0
                                width: 270
                            }  
                            ConfigSlider {
                                label: "Green"
                                integral: false
                                config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
                                property: "colorG"
                                max: 1.0
                                min: 0.0
                                width: 270
                            }  
                            ConfigSlider {
                                label: "Blue"
                                integral: false
                                config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
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
                                config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
                                property: "unoccludedFillOpacity"
                                max: 1.0
                                min: 0.0
                                width: 270
                            }  
                            ConfigSlider {
                                label: "Occluded"
                                integral: false
                                config: Render.getConfig("RenderMainView.OutlineEffect"+tabs.currentIndex)
                                property: "occludedFillOpacity"
                                max: 1.0
                                min: 0.0
                                width: 270
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        for (var i=0 ; i<4 ; i++) {
            var outlinePage = tabs.addTab("Outl. "+i, paramWidgets)
            outlinePage.active = true
        }
    }
}
