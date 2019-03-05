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
    //    "type": "PropXXXX", "object": object, "property": "propName"      
    // }
    //
    property var propItems: []


    property var label: "group"

      /*  Component.onCompleted: {
            var component1 = Qt.createComponent("PropBool.qml");
                    component1.label = "Test";
            for (var i=0; i<root.propItems.length; i++) {

            //  if (propItems[i]["type"] == "PropBool") {
                    var component = Qt.createComponent("PropBool.qml");
                    component.label = propItems[i]["property"];

            // }
            }
        }*/

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

        Repeater {
            model: root.propItems
            PropBool {
                    label: qsTr(modelData["property"])
                    object: modelData["object"]
                    property: modelData["property"]
                    anchors.left: parent.left
                    anchors.right: parent.right 
            }
        }   
    }

    height: column.height
}
