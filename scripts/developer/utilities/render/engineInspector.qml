//
//  EngineInspector.qml
//
//  Created by Sam Gateau on 06/07/2018
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3


import "../lib/jet/qml" as Jet

Item {
    anchors.fill: parent 
    id: root;   
    
    property var rootConfig: Render.getConfig("")

    ScrollView {
        id: scrollView
        anchors.fill: parent 
        contentWidth: parent.width
        clip: true

        Column {
            anchors.left: parent.left 
            anchors.right: parent.right 

            Jet.TaskPropView {
                rootConfig: root.rootConfig
                anchors.fill: root        
            }
        }
    }
}