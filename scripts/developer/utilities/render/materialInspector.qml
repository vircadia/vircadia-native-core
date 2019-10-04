//
//  materialInspector.qml
//
//  Created by Sabrina Shanman on 2019-01-16
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.3 as Original
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import "../lib/prop" as Prop

Rectangle {
    HifiConstants { id: hifi;}
    color: Qt.rgba(hifi.colors.baseGray.r, hifi.colors.baseGray.g, hifi.colors.baseGray.b, 0.8);
    id: root;
    
    property var theMaterial: {}
    property var theMaterialAttributes: {}
    property var hasMaterial: false

    property var isReadOnly: true

    function fromScript(message) {
        switch (message.method) {
        case "setObjectInfo":
            entityIDInfo.text = "Type: " + message.params.type + "\nID: " + message.params.id + "\nMesh Part: " + message.params.meshPart;
            break;
        case "setMaterialJSON":
            materialJSONText.text = message.params.materialJSONText;
            
            theObject = JSON.parse(message.params.materialJSONText)
            theMaterialAttributes = theObject.materials
            console.log(JSON.stringify(theOtheMaterialAttributesbject))
            hasMaterial = (theMaterial !== undefined)
            break;
        }
    }
  
    Column {  
        
        anchors.left: parent.left 
        anchors.right: parent.right 

        Rectangle {
            id: entityIDContainer
            height: 52
            width: root.width
            color: Qt.rgba(root.color.r * 0.7, root.color.g * 0.7, root.color.b * 0.7, 0.8);
            TextEdit {
                id: entityIDInfo
                text: "Type: Unknown\nID: None\nMesh Part: Unknown"
                font.pointSize: 9
                color: "#FFFFFF"
                readOnly: true
                selectByMouse: true
            }
        }
    
        Prop.PropScalar {
            visible: hasMaterial && ("roughness" in theMaterialAttributes)
            label: "roughness"
            object: theMaterialAttributes
            property: "roughness"
            readOnly: isReadOnly
        }
        Prop.PropScalar {
            visible: hasMaterial && ("opacity" in theMaterialAttributes)
            label: "opacity"
            object: theMaterialAttributes
            property: "opacity"
            readOnly: isReadOnly
        }

        Prop.PropString {
            visible: hasMaterial && ("albedoMap" in theMaterialAttributes)
            label: "roughness"
            object: theMaterialAttributes
            property: "roughness"
            readOnly: isReadOnly
        }

        Original.ScrollView {
           // anchors.top: entityIDContainer.bottom
            height: root.height - entityIDContainer.height
            width: root.width
            clip: true
            Original.ScrollBar.horizontal.policy: Original.ScrollBar.AlwaysOff
            TextEdit {
                id: materialJSONText
                text: "Click an object to get material JSON"
                width: root.width
                font.pointSize: 10
                color: "#FFFFFF"
                readOnly: true
                selectByMouse: true
                wrapMode: Text.WordWrap
            }
        }
    }
}
