//
//  BoundingBoxes.qml
//
//  Created by Sam Gateau on 4/18/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import "../../lib/prop" as Prop

Column {

    id: root;   
    
    property var mainViewTask: Render.getConfig("RenderMainView")
    
    spacing: 5
    anchors.left: parent.left
    anchors.right: parent.right       
    anchors.margins: hifi.dimensions.contentMargin.x  

    Row {
        anchors.left: parent.left
        anchors.right: parent.right 
            
        spacing: 5 
        Column {
            spacing: 5 

            Prop.PropCheckBox {
                text: "Opaques"
                checked: root.mainViewTask.getConfig("DrawOpaqueBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawOpaqueBounds")["enabled"] = checked }
            }
            Prop.PropCheckBox {
                text: "Transparents"
                checked: root.mainViewTask.getConfig("DrawTransparentBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawTransparentBounds")["enabled"] = checked }
            }
            Prop.PropCheckBox {
                text: "Metas"
                checked: root.mainViewTask.getConfig("DrawMetaBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawMetaBounds")["enabled"] = checked }
            }   
            Prop.PropCheckBox {
                text: "Lights"
                checked: root.mainViewTask.getConfig("DrawLightBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawLightBounds")["enabled"] = checked; }
            }
            Prop.PropCheckBox {
                text: "Zones"
                checked: root.mainViewTask.getConfig("DrawZones")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawZones")["enabled"] = checked; }
            }  
        }
        Column {
            spacing: 5 
            Prop.PropCheckBox {
                text: "Opaques in Front"
                checked: root.mainViewTask.getConfig("DrawOverlayInFrontOpaqueBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawOverlayInFrontOpaqueBounds")["enabled"] = checked }
            }
            Prop.PropCheckBox {
                text: "Transparents in Front"
                checked: root.mainViewTask.getConfig("DrawOverlayInFrontTransparentBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawOverlayInFrontTransparentBounds")["enabled"] = checked }
            }
            Prop.PropCheckBox {
                text: "Opaques in HUD"
                checked: root.mainViewTask.getConfig("DrawOverlayHUDOpaqueBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawOverlayHUDOpaqueBounds")["enabled"] = checked }
            }
            Prop.PropCheckBox {
                text: "Transparents in HUD"
                checked: root.mainViewTask.getConfig("DrawOverlayHUDTransparentBounds")["enabled"]
                onCheckedChanged: { root.mainViewTask.getConfig("DrawOverlayHUDTransparentBounds")["enabled"] = checked }
            }
        }
    }
}
