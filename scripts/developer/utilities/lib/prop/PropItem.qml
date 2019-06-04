//
//  PropItem.qml
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

    // Prop item is designed to author an object[property]:
    property var object: {}
    property string property: ""
    property bool readOnly: false

    // value is accessed through the "valueVarSetter" and "valueVarGetter"
    // By default, these just go get or set the value from the object[property]
    // 
    function defaultGet() { return root.object[root.property]; }
    function defaultSet(value) { root.object[root.property] = value; }  
   // function defaultSetReadOnly(value) { log ( "read only " + property + ", NOT setting to " + value); }  
 //   function defaultSetReadOnly(value) {}  
  //  property var valueVarSetter: (root.readOnly ? defaultSetReadOnly : defaultSet)
    property var valueVarSetter: defaultSet
    property var valueVarGetter: defaultGet

    // PropItem is stretching horizontally accross its parent
    // Fixed height
    anchors.left: parent.left
    anchors.right: parent.right    
    height: global.lineHeight
    anchors.leftMargin: global.horizontalMargin
    anchors.rightMargin: global.horizontalMargin

    // LabelControl And SplitterControl are on the left side of the PropItem
    property bool showLabel: true  
    property alias labelControl: labelControl
    property alias label: labelControl.text
    
    property var labelAreaWidth: root.width * global.splitterRightWidthScale - global.splitterWidth
 
    PropText {
        id: labelControl
        text: root.label
        enabled: root.showLabel

        anchors.left: root.left
        anchors.verticalCenter: root.verticalCenter
        width: labelAreaWidth
    }

    property alias splitter: splitterControl   
    PropSplitter {
        id: splitterControl
        
        anchors.left: labelControl.right
        size: global.splitterWidth
    }

}
