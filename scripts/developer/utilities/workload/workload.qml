//
//  workload.qml
//
//  Created by Sam Gateau on 3/1/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
import  "../render/configSlider"

Rectangle {
    HifiConstants { id: hifi;}
    id: workload;   
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;
    property var spaceToRender: Workload.getConfig("SpaceToRender")
   
    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  
        //padding: hifi.dimensions.contentMargin.x

        HifiControls.Label {
            text: "Workload"       
        }
        Separator {}
        HifiControls.Label {
            text: "Display"       
        }
        HifiControls.CheckBox {
            boxSize: 20
            text: "Show Proxies"
            checked: workload.spaceToRender["showProxies"]
            onCheckedChanged: { workload.spaceToRender["showProxies"] = checked }
        }
        HifiControls.CheckBox {
            boxSize: 20
            text: "Show Views"
            checked: workload.spaceToRender["showViews"]
            onCheckedChanged: { workload.spaceToRender["showViews"] = checked }
        }
        Separator {}
    }
}
