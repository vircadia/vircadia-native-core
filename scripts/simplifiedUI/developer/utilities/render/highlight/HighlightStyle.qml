//
//  highlightStyle.qml
//
//  Created by Sam Gateau 12/4/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "../configSlider"
import "../../lib/plotperf"
import stylesUit 1.0
import controlsUit 1.0 as HifiControls

Item {
    id: root
    property var highlightStyle
    height: 48

    anchors.margins: 0

    signal newStyle()

    function getStyle() {
        return highlightStyle;
    }

    Component.onCompleted: {
    }

    Column {
        spacing: 5
        anchors.left: root.left
        anchors.right: root.right       
        anchors.margins: 0  


        
        ConfigSlider {
            label: "Outline Width"
            integral: false
            config: root.highlightStyle
            property: "outlineWidth"
            max: 10
            min: 0

            anchors.left: parent.left
            anchors.right: parent.right 

            onValueChanged: { root.highlightStyle["outlineWidth"] = value; newStyle() }
        }
        HifiControls.CheckBox {
            id: isOutlineSmooth
            text: "Smooth Outline"
            checked: root.highlightStyle["isOutlineSmooth"]
            onCheckedChanged: {
                root.highlightStyle["isOutlineSmooth"] = checked;
                newStyle();
            }
        }
        
        Repeater {
            model: [
                "Outline Unoccluded:outlineUnoccludedColor:outlineUnoccludedAlpha",
                "Outline Occluded:outlineOccludedColor:outlineOccludedAlpha",
                "Fill Unoccluded:fillUnoccludedColor:fillUnoccludedAlpha",
                "Fill Occluded:fillOccludedColor:fillOccludedAlpha"]
            Column {
                    anchors.left: parent.left
                    anchors.right: parent.right 
                    anchors.margins: 0  
           
            Color {
                height: 20
                anchors.right: parent.right       
                width: root.width / 2
                _color: Qt.rgba(root.highlightStyle[modelData.split(":")[1]].red / 255, root.highlightStyle[modelData.split(":")[1]].green / 255, root.highlightStyle[modelData.split(":")[1]].blue / 255, 1.0)
                onNewColor: {
                    root.highlightStyle[modelData.split(":")[1]] = getXColor()
                    newStyle()
                }
            }
            
            ConfigSlider {
                label: qsTr(modelData.split(":")[0])
                integral: false
                config: root.highlightStyle
                property: modelData.split(":")[2]
                max: 1.0
                min: 0.0

                anchors.left: parent.left
                anchors.right: parent.right 

                onValueChanged: { root.highlightStyle[modelData.split(":")[2]] = value; newStyle() }
            }

            }
        }

    }
}
