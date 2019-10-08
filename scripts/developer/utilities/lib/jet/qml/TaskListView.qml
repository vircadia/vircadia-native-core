//
//  jet/TaskListView.qml
//
//  Created by Sam Gateau, 2018/05/09
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../jet.js" as Jet

Rectangle {
    HifiConstants { id: hifi;}
    color: Qt.rgba(hifi.colors.baseGray.r, hifi.colors.baseGray.g, hifi.colors.baseGray.b, 0.8);
    id: root;
    
    property var rootConfig : Workload
    

    Component.onCompleted: {
        var functor = Jet.job_tree_model_functor(jobsModel, 3, function(node) {
              node["cpuT"] = 0.0
        })
        Jet.task_traverseTree(rootConfig, functor);
    }
        
    
    ListModel {
        id: jobsModel
        property var engineJobItemModel : []
    }

    Component {
        id: objRecursiveDelegate
        Column {
            id: objRecursiveColumn
            clip: true
            visible: model.init
   
            function switchFold() {
                for(var i = 1; i < children.length - 1; ++i) {
                    children[i].visible = !children[i].visible
                }
            }
            
            Row {
                id: objRow
                Item {
                    height: 1
                    width: model.level * 15
                }

                HifiControls.CheckBox {
                    id: objCheck
                    property var config: root.rootConfig.getConfig(model.path + "." + model.name);
                    text: " "
                    checked: root.rootConfig.getConfig(model.path + "." + model.name).enabled
                    onCheckedChanged: { root.rootConfig.getConfig(model.path + "." + model.name).enabled = checked }
                }

                MouseArea {
                    width: objLabel.implicitWidth
                    height: objLabel.implicitHeight 
                    onDoubleClicked: {
                        parent.parent.switchFold()
                    }

                    HifiControls.Label {
                        id: objLabel
                        colorScheme: (root.rootConfig.getConfig(model.path + "." + model.name) ? hifi.colorSchemes.dark : hifi.colorSchemes.light)
                        text: (objRecursiveColumn.children.length > 2 ?
                                objRecursiveColumn.children[1].visible ?
                                qsTr("-  ") : qsTr("+ ") : qsTr("   ")) + model.name
                                + " id=" + model.id
                    }
                }
            }

            Repeater {
                model: subNode
                delegate: objRecursiveDelegate
            }
        }
    }

    ListView {
        anchors.fill: parent 

        id: theView
        model: jobsModel
        delegate: objRecursiveDelegate
    }
    
}