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
            
            theMaterial = JSON.parse(message.params.materialJSONText)
            theMaterialAttributes = theMaterial.materials
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
    
        Prop.PropString {
            visible: hasMaterial && ("name" in theMaterialAttributes)
            label: "name"
            object: theMaterialAttributes
            property: "name"
            readOnly: isReadOnly
        } 
        Prop.PropString {
            visible: hasMaterial && ("model" in theMaterialAttributes)
            label: "model"
            object: theMaterialAttributes
            property: "model"
            readOnly: isReadOnly
        } 

        Prop.PropColor {
            visible: hasMaterial && ("albedo" in theMaterialAttributes)
            label: "albedo"
            object: theMaterialAttributes
            property: "albedo"
            readOnly: isReadOnly
        }       
        Prop.PropString {
            visible: hasMaterial && ("albedoMap" in theMaterialAttributes)
            label: "albedoMap"
            object: theMaterialAttributes
            property: "albedoMap"
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
            visible: hasMaterial && ("opacityMap" in theMaterialAttributes)
            label: "opacityMap"
            object: theMaterialAttributes
            property: "opacityMap"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("opacityMapMode" in theMaterialAttributes)
            label: "opacityMapMode"
            object: theMaterialAttributes
            property: "opacityMapMode"
            readOnly: isReadOnly
        }
        /*Prop.PropEnum {
            visible: hasMaterial && ("opacityMapMode" in theMaterialAttributes)
            label: "opacityMapMode"
            object: theMaterialAttributes
            property: "opacityMapMode"
            readOnly: isReadOnly
            enums: ["None", "Mask", "Blend"]
        } */
        Prop.PropScalar {
            visible: hasMaterial && ("opacityCutoff" in theMaterialAttributes)
            label: "opacity Cutoff"
            object: theMaterialAttributes
            property: "opacityCutoff"
            readOnly: isReadOnly
        }
        
        Prop.PropString {
            visible: hasMaterial && ("occlusionMap" in theMaterialAttributes)
            label: "occlusionMap"
            object: theMaterialAttributes
            property: "occlusionMap"
            readOnly: isReadOnly
        }        
        Prop.PropString {
            visible: hasMaterial && ("normalMap" in theMaterialAttributes)
            label: "normalMap"
            object: theMaterialAttributes
            property: "normalMap"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("bumpMap" in theMaterialAttributes)
            label: "normalMap from bumpMap"
            object: theMaterialAttributes
            property: "bumpMap"
            readOnly: isReadOnly
        } 

        Prop.PropScalar {
            visible: hasMaterial && ("roughness" in theMaterialAttributes)
            label: "roughness"
            object: theMaterialAttributes
            property: "roughness"
            readOnly: isReadOnly
        }
        Prop.PropScalar {
            visible: hasMaterial && ("metallic" in theMaterialAttributes)
            label: "metallic"
            object: theMaterialAttributes
            property: "metallic"
            readOnly: isReadOnly
        }           
        Prop.PropString {
            visible: hasMaterial && ("roughnessMap" in theMaterialAttributes)
            label: "roughnessMap"
            object: theMaterialAttributes
            property: "roughnessMap"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("glossMap" in theMaterialAttributes)
            label: "roughnessMap from glossMap"
            object: theMaterialAttributes
            property: "glossMap"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("metallicMap" in theMaterialAttributes)
            label: "metallicMap"
            object: theMaterialAttributes
            property: "metallicMap"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("specularMap" in theMaterialAttributes)
            label: "metallicMap from specularMap"
            object: theMaterialAttributes
            property: "specularMap"
            readOnly: isReadOnly
        }

        Prop.PropScalar {
            visible: hasMaterial && ("scattering" in theMaterialAttributes)
            label: "scattering"
            object: theMaterialAttributes
            property: "scattering"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("scatteringMap" in theMaterialAttributes)
            label: "scatteringMap"
            object: theMaterialAttributes
            property: "scatteringMap"
            readOnly: isReadOnly
        }


        Prop.PropColor {
            visible: hasMaterial && ("emissive" in theMaterialAttributes)
            label: "emissive"
            object: theMaterialAttributes
            property: "emissive"
            readOnly: isReadOnly
        }
        Prop.PropString {
            visible: hasMaterial && ("emissiveMap" in theMaterialAttributes)
            label: "emissiveMap"
            object: theMaterialAttributes
            property: "emissiveMap"
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
