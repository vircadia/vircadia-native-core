//
//  cash.qml
//
//  Created by Sam Gateau on 17/9/2019
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
import "cash"
import "../lib/plotperf"

Rectangle {
    anchors.fill: parent 
    id: root;   
    
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
                label: "Resource Queries Inspector"
                isUnfold: true
                panelFrameData: Component {
                    Column {
                        PlotPerf {
                        title: "Global Queries"
                        height: 80
                        valueScale: 1
                        valueUnit: ""
                        plots: [
                        {
                            object: ModelCache,
                            prop: "numGlobalQueriesPending",
                            label: "Pending",
                            color: "#1AC567"
                        },
                        {
                            object: ModelCache,
                            prop: "numGlobalQueriesLoading",
                            label: "Loading",
                            color: "#FEC567"
                        },
                        {
                            object: ModelCache,
                            prop: "numLoading",
                            label: "Model Loading",
                            color: "#C5FE67"
                        }
                        ]
                        }
                    }
                }
            }

            Prop.PropFolderPanel {
                label: "Cache Inspectors"
                isUnfold: true
                panelFrameData: Component {
                    Column {
                        Prop.PropButton {
                            text: "Model"
                            onClicked: {
                                sendToScript({method: "openModelCacheInspector"}); 
                            }
                            width:column.width
                        }
                        Prop.PropButton {
                            text: "Material"
                            onClicked: {
                                sendToScript({method: "openMaterialCacheInspector"}); 
                            }
                            width:column.width
                        }
                        Prop.PropButton {
                            text: "Texture"
                            onClicked: {
                                sendToScript({method: "openTextureCacheInspector"}); 
                            }
                            width:column.width
                        }
                        Prop.PropButton {
                            text: "Animation"
                            onClicked: {
                                sendToScript({method: "openAnimationCacheInspector"}); 
                            }
                            width:column.width
                        }
                        Prop.PropButton {
                            text: "Sound"
                            onClicked: {
                                sendToScript({method: "openSoundCacheInspector"}); 
                            }
                            width:column.width
                        }
                    }
                }
            }
            Prop.PropFolderPanel {
                label: "Stats"
                isUnfold: true
                panelFrameData: Component { Column {
                    PlotPerf {
                        title: "Resources"
                        height: 200
                        valueScale: 1
                        valueUnit: ""
                        plots: [
                        {
                            object: TextureCache,
                            prop: "numTotal",
                            label: "Textures",
                            color: "#1AC567"
                        },
                        {
                            object: TextureCache,
                            prop: "numCached",
                            label: "Textures Cached",
                            color: "#FEC567"
                        },
                        {
                            object: ModelCache,
                            prop: "numTotal",
                            label: "Models",
                            color: "#FED959"
                        },
                        {
                            object: ModelCache,
                            prop: "numCached",
                            label: "Models Cached",
                            color: "#FEFE59"
                        },
                        {
                            object: MaterialCache,
                            prop: "numTotal",
                            label: "Materials",
                            color: "#00B4EF"
                        },
                        {
                            object: MaterialCache,
                            prop: "numCached",
                            label: "Materials Cached",
                            color: "#FFB4EF"
                        }
                        ]
                    }}
                }
            }
        }
    }
}