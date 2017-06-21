//
//  culling.qml
//  examples/utilities/render
//
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
    id: root
    spacing: 8
    property var sceneOctree: Render.getConfig("RenderMainView.DrawSceneOctree");
    property var itemSelection: Render.getConfig("RenderMainView.DrawItemSelection");

     Component.onCompleted: {
        sceneOctree.enabled = true;
        itemSelection.enabled = true;                    
        sceneOctree.showVisibleCells = false;
        sceneOctree.showEmptyCells = false;
        itemSelection.showInsideItems = false;
        itemSelection.showInsideSubcellItems = false;
        itemSelection.showPartialItems = false;
        itemSelection.showPartialSubcellItems = false;
    }
    Component.onDestruction: {
        sceneOctree.enabled = false;
        itemSelection.enabled = false;  
        Render.getConfig("RenderMainView.FetchSceneSelection").freezeFrustum = false;
        Render.getConfig("RenderMainView.CullSceneSelection").freezeFrustum = false;                     
    }

    GroupBox {
        title: "Culling"
        Row {
            spacing: 8
            Column {
                spacing: 8

                CheckBox {
                    text: "Freeze Culling Frustum"
                    checked: false
                    onCheckedChanged: { 
                        Render.getConfig("RenderMainView.FetchSceneSelection").freezeFrustum = checked;
                        Render.getConfig("RenderMainView.CullSceneSelection").freezeFrustum = checked;
                    }
                }
                Label {
                    text: "Octree"
                }
                CheckBox {
                    text: "Visible Cells"
                    checked: root.sceneOctree.showVisibleCells
                    onCheckedChanged: { root.sceneOctree.showVisibleCells = checked }
                }
                CheckBox {
                    text: "Empty Cells"
                    checked: false
                    onCheckedChanged: { root.sceneOctree.showEmptyCells = checked }
                }
            }
            Column {
                spacing: 8

                Label {
                    text: "Frustum Items"
                }
                CheckBox {
                    text: "Inside Items"
                    checked: false
                    onCheckedChanged: { root.itemSelection.showInsideItems = checked }
                }
                CheckBox {
                    text: "Inside Sub-cell Items"
                    checked: false
                    onCheckedChanged: { root.itemSelection.showInsideSubcellItems = checked }
                }
                CheckBox {
                    text: "Partial Items"
                    checked: false
                    onCheckedChanged: { root.itemSelection.showPartialItems = checked }
                }
                CheckBox {
                    text: "Partial Sub-cell Items"
                    checked: false
                    onCheckedChanged: { root.itemSelection.showPartialSubcellItems = checked }
                } 
            }
        }
    }   

    GroupBox {
        title: "Render Items"

        Column{
            Repeater {
                model: [ "Opaque:RenderMainView.DrawOpaqueDeferred", "Transparent:RenderMainView.DrawTransparentDeferred", "Light:RenderMainView.DrawLight",
                        "Opaque Overlays:RenderMainView.DrawOverlay3DOpaque", "Transparent Overlays:RenderMainView.DrawOverlay3DTransparent" ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: true
                    config: Render.getConfig(modelData.split(":")[1])
                    property: "maxDrawn"
                    max: config.numDrawn
                    min: -1
                }
            }
        }
    }
}
