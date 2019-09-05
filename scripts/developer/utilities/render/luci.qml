//
//  luci.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import controlsUit 1.0 as HifiControls

import "../lib/prop" as Prop
import "../lib/jet/qml" as Jet
import "luci"

Rectangle {
    anchors.fill: parent 
    id: render;   
    property var mainViewTask: Render.getConfig("RenderMainView")
    
    Prop.Global { id: global;}
    color: global.color

    ScrollView {
        id: scrollView
        anchors.fill: parent 
        contentWidth: parent.width
        clip: true
         
        Column {
            id: column
            width: parent.width
            Prop.PropFolderPanel {
                label: "Render Settings"
                isUnfold: false
                panelFrameData: Component {
                    RenderSettings {
                    }
                }
            }
            Prop.PropFolderPanel {
                label: "Shading Model"
                panelFrameData: Component {
                    ShadingModel {}
                }
            }
            Prop.PropFolderPanel {
                label: "Bounding Boxes"
                panelFrameData: Component {
                    BoundingBoxes {}
                }
            }
            Prop.PropFolderPanel {
                label: "Framebuffer"
                panelFrameData: Component {
                    Framebuffer {}
                }
            }
            Prop.PropFolderPanel {
                label: "Tone Mapping"
                panelFrameData: Component {
                    ToneMapping {}
                }
            }
            Prop.PropFolderPanel {
                label: "Antialiasing"
                panelFrameData: Component {
                    Antialiasing {}
                }
            }
            Prop.PropFolderPanel {
                label: "Bloom"
                panelFrameData: Component {
                    Bloom {}
                }
            }
            Prop.PropFolderPanel {
                label: "Culling"
                panelFrameData: Component {
                    Culling {}
                }
            }
            Prop.PropFolderPanel {
                label: "Tools"
                panelFrameData: Component {
                    Row {
                        HifiControls.Button {
                            text: "Engine"
                            onClicked: {
                                sendToScript({method: "openEngineInspectorView"}); 
                            }
                            width:column.width / 3
                        }
                        HifiControls.Button {
                            text: "LOD"
                            onClicked: {
                                sendToScript({method: "openEngineLODView"}); 
                            }
                            width:column.width / 3
                        }
                        HifiControls.Button {
                            text: "Material"
                            onClicked: {
                                sendToScript({method: "openMaterialInspectorView"}); 
                            }
                            width:column.width / 3
                        }
                    }
                }
            }
      /*      Jet.TaskPropView {
                id: "le"
                jobPath: ""
                label: "Le Render Engine"

              //  anchors.left: parent.left
              //  anchors.right: parent.right 
            }*/
        }
    }
}