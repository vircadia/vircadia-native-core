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


    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right   
    
        PropLabel {
            anchors.left: parent.left
            anchors.right: parent.right
            text: root.label
            horizontalAlignment: Text.AlignHCenter
        }
      

    }
    height: column.height

    Component.onCompleted: {
        for (var i = 0; i < root.propItems.length; i++) {
            var proItem = root.propItems[i];
            switch(proItem.type) {
                case 'PropBool': {
                    var component = Qt.createComponent("PropBool.qml");
                    component.createObject(column, {
                        "label": proItem.property,
                        "object": proItem.object,
                        "property": proItem.property
                    })
                } break;
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
            }      
        }
    }
}
