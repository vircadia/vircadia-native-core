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
        }
    }
}