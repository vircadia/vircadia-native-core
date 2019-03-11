//
//  PropGroup.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

Item {
    Global { id: global }
    id: root
    
    // Prop Group is designed to author an array of ProItems, they are defined with an array of the tuplets describing each individual item:
    // [ ..., PropItemInfo, ...]
    // PropItemInfo {
    //    type: "PropXXXX", object: JSobject, property: "propName"      
    // }
    //
    property var propItems: []


    property var label: "group"

    property alias isUnfold: headerRect.icon
    
    Item {
        id: header
        height: global.slimHeight
        anchors.left: parent.left           
        anchors.right: parent.right

        Item {
            id: folder
            anchors.left: header.left
            width: headerRect.width * 2
            anchors.verticalCenter: header.verticalCenter
            height: parent.height
            
            PropCanvasIcon {
                id: headerRect
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                 
                MouseArea{
                    id: mousearea
                    anchors.fill: parent
                    onClicked: {
                        root.isUnfold = !root.isUnfold
                    }
                }
            }
        }

        PropLabel {
            id: labelControl
            anchors.left: folder.right
            anchors.right: header.right
            anchors.verticalCenter: header.verticalCenter
            text: root.label
            horizontalAlignment: Text.AlignHCenter
        }

     /*   Rectangle {
            anchors.left: parent.left           
            anchors.right: parent.right
            height: 1
            anchors.bottom: parent.bottom
            color: global.colorBorderHighight
       
            visible: root.isUnfold   
        }*/
    }

    Rectangle {
        visible: root.isUnfold

        color: "transparent"
        border.color: global.colorBorderLight
        border.width: global.valueBorderWidth
        radius: global.valueBorderRadius

        anchors.left: parent.left           
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: root.bottom  

        Column {
            id: column
          //  anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right   
            clip: true

            // Where the propItems are added
        }
    }

    height: header.height + isUnfold * column.height
    anchors.leftMargin: global.horizontalMargin
    anchors.rightMargin: global.horizontalMargin

    function updatePropItems() {
         for (var i = 0; i < root.propItems.length; i++) {
            var proItem = root.propItems[i];
            // valid object
            if (proItem['object'] !== undefined && proItem['object'] !== null ) {
                // valid property
                if (proItem['property'] !== undefined && proItem.object[proItem.property] !== undefined) {
                    // check type
                    if (proItem['type'] === undefined) {
                        proItem['type'] = typeof(proItem.object[proItem.property])
                    }
                    switch(proItem.type) {
                        case 'boolean':
                        case 'PropBool': {
                            var component = Qt.createComponent("PropBool.qml");
                            component.createObject(column, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property
                            })
                        } break;
                        case 'number':
                        case 'PropScalar': {
                            var component = Qt.createComponent("PropScalar.qml");
                            component.createObject(column, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property,
                                "min": (proItem["min"] !== undefined ? proItem.min : 0.0),                   
                                "max": (proItem["max"] !== undefined ? proItem.max : 1.0),                                       
                                "integer": (proItem["integral"] !== undefined ? proItem.integral : false),
                            })
                        } break;
                        case 'PropEnum': {
                            var component = Qt.createComponent("PropEnum.qml");
                            component.createObject(column, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property,
                                "enums": (proItem["enums"] !== undefined ? proItem.enums : ["Undefined Enums !!!"]), 
                            })
                        } break;
                        case 'object': {
                            var component = Qt.createComponent("PropItem.qml");
                            component.createObject(column, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property,
                             })
                        } break;
                    }
                } else {
                    console.log('Invalid property: ' + JSON.stringify(proItem));
                }
            } else {
                console.log('Invalid object: ' + JSON.stringify(proItem));
            }
        }
    }
    Component.onCompleted: {
        updatePropItems();
    }
}
